// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2015-2020 The Linux Foundation. All rights reserved.
//               2023      The LineageOS Project
//

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#include <sound/soc.h>
#include <asoc/msm-cdc-pinctrl.h>
#include "msm8952.h"

struct cdc_pdm_pinctrl_info {
	struct pinctrl *pinctrl;
	struct pinctrl_state *spk_ext_pa_act;
	struct pinctrl_state *spk_ext_pa_sus;
};

static struct cdc_pdm_pinctrl_info pinctrl_info;

int msm_spk_ext_pa_ctrl(struct msm_asoc_mach_data *pdata, bool value)
{
	struct sched_param param;
	bool on_off = !value;
	int ret = 0;
	int maxpri;
	int i, pulse_count;

	if (!pdata || !gpio_is_valid(pdata->spk_ext_pa_gpio_lc)) {
		pr_debug("%s: spk_ext_pa_gpio_lc is invalid\n", __func__);
		return -EINVAL;
	}

	maxpri = MAX_USER_RT_PRIO - 1;
	param.sched_priority = maxpri;
	if (sched_setscheduler(current, SCHED_FIFO, &param) == -1)
		pr_debug("%s: sched_setscheduler failed\n", __func__);

	pr_debug("%s: pa_is_on=%d, spk_ext_pa_gpio_lc=%d, on_off=%d\n",
		 __func__, pdata->pa_is_on, pdata->spk_ext_pa_gpio_lc, on_off);

	ret = msm_cdc_pinctrl_select_active_state(pdata->mi2s_gpio_p[PRIM_MI2S]);
	if (ret) {
		pr_err("%s: gpio set cannot be de-activated pri_i2s\n", __func__);
		return ret;
	}

	if (on_off) {
		pulse_count = pdata->ext_pa_mode ? pdata->ext_pa_mode : 1;

		gpio_direction_output(pdata->spk_ext_pa_gpio_lc, 0);
		mdelay(2);

		for (i = 0; i < pulse_count; i++) {
			gpio_set_value(pdata->spk_ext_pa_gpio_lc, 0);
			udelay(2);
			gpio_set_value(pdata->spk_ext_pa_gpio_lc, 1);
			udelay(2);
		}

		msleep(3);
		pr_debug("%s: PA Opened successfully with %d pulses\n", __func__, pulse_count);
	} else {
		gpio_set_value(pdata->spk_ext_pa_gpio_lc, 0);
		pr_debug("%s: PA Closed successfully\n", __func__);
	}

	return ret;
}

void msm_spk_ext_pa_delayed(struct work_struct *work)
{
	struct delayed_work *dwork;
	struct msm_asoc_mach_data *pdata;

	dwork = to_delayed_work(work);
	pdata = container_of(dwork, struct msm_asoc_mach_data, pa_gpio_work);
	pr_debug("At %d In (%s), enter, pdata->pa_is_on=%d\n", __LINE__, __FUNCTION__, pdata->pa_is_on);

	if (!pdata->pa_is_on) {
		pr_debug("At %d In (%s), open pa\n", __LINE__, __FUNCTION__);
		msm_spk_ext_pa_ctrl(pdata, true);
		pdata->pa_is_on = 2;
	}
}

int msm_setup_spk_ext_pa(struct platform_device *pdev, struct msm_asoc_mach_data *pdata)
{
	struct pinctrl *pinctrl;

	pdata->spk_ext_pa_gpio_lc = of_get_named_gpio_flags(pdev->dev.of_node,
					"qcom,spk_ext_pa", 0, NULL);
	if (pdata->spk_ext_pa_gpio_lc < 0) {
		pr_debug("%s: spk_ext_pa_gpio_lc not exist!\n", __func__);
		return 0;
	}

	if (of_property_read_u32(pdev->dev.of_node, "qcom,ext-pa-mode", &pdata->ext_pa_mode))
		pdata->ext_pa_mode = 1;

	pr_debug("%s: spk_ext_pa_gpio_lc=%d, ext_pa_mode=%d\n",
		 __func__, pdata->spk_ext_pa_gpio_lc, pdata->ext_pa_mode);
	pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		pr_err("%s: Unable to get pinctrl handle\n", __func__);
		return -EINVAL;
	}
	pinctrl_info.pinctrl = pinctrl;
	/* get pinctrl handle for spk_ext_pa_gpio_lc */
	pinctrl_info.spk_ext_pa_act = pinctrl_lookup_state(pinctrl, "spk_ext_pa_active");
	if (IS_ERR(pinctrl_info.spk_ext_pa_act)) {
		pr_err("%s: Unable to get pinctrl active handle\n", __func__);
		return -EINVAL;
	}
	pinctrl_info.spk_ext_pa_sus = pinctrl_lookup_state(pinctrl, "spk_ext_pa_suspend");
	if (IS_ERR(pinctrl_info.spk_ext_pa_sus)) {
		pr_err("%s: Unable to get pinctrl disable handle\n", __func__);
		return -EINVAL;
	}
	if (gpio_is_valid(pdata->spk_ext_pa_gpio_lc)) {
		pr_debug("%s, spk_ext_pa_gpio_lc request\n", __func__);
			pr_debug("At %d In (%s), set spk_ext_pa_gpio_lc to low\n", __LINE__, __FUNCTION__);
		gpio_direction_output(pdata->spk_ext_pa_gpio_lc, 0);
		mdelay(2);
		gpio_set_value(pdata->spk_ext_pa_gpio_lc, 0);
	}
	return 0;
}

static int external_spk_control = 1;

int get_external_spk_pa(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = external_spk_control;
	return 0;
}

int set_external_spk_pa(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct msm_asoc_mach_data *pdata = NULL;

	if (!component)
		return -EINVAL;

	pdata = snd_soc_card_get_drvdata(component->card);
	if (!pdata)
		return -EINVAL;

	if (external_spk_control == ucontrol->value.integer.value[0])
		return 0;

	external_spk_control = ucontrol->value.integer.value[0];
	msm_spk_ext_pa_ctrl(pdata, external_spk_control);
	return 1;
}