// SPDX-License-Identifier: GPL-2.0-only
/*
 * Qualcomm MSM8953 CPU and CCI clocks.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_clk.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_opp.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/regulator/rpm-smd-regulator.h>

#include <dt-bindings/clock/qcom,cpu-msm8953.h>

#include "clk-alpha-pll.h"
#include "clk-regmap-mux-div.h"
#include "common.h"

#define MSM8953_PLL_RATE_MIN	652800000UL
#define MSM8953_PLL_RATE_MAX	2400000000UL
#define MSM8953_BOOT_RATE	883200000UL
#define MSM8953_CCI_DIVIDER	5

enum msm8953_cpu_parent {
	P_XO,
	P_GPLL0,
	P_APCS_PLL,
};

static const struct parent_map cpu_parent_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 4 },
	{ P_APCS_PLL, 5 },
};

static const char * const cpu_parent_names[] = {
	"bi_tcxo_ao", "gpll0", "apcs_hf_pll",
};

static const struct parent_map cci_parent_map[] = {
	{ P_APCS_PLL, 5 },
};

static const char * const cci_parent_names[] = {
	"apcs_hf_pll",
};

static const u8 msm8953_pll_regs[PLL_OFF_MAX_REGS] = {
	[PLL_OFF_L_VAL] = 0x08,
	[PLL_OFF_ALPHA_VAL] = 0x10,
	[PLL_OFF_ALPHA_VAL_U] = 0x14,
	[PLL_OFF_USER_CTL] = 0x18,
	[PLL_OFF_CONFIG_CTL] = 0x20,
	[PLL_OFF_CONFIG_CTL_U] = 0x24,
	[PLL_OFF_TEST_CTL] = 0x30,
	[PLL_OFF_TEST_CTL_U] = 0x34,
	[PLL_OFF_STATUS] = 0x28,
};

static int msm8953_pll_corner[] = {
	RPM_REGULATOR_LEVEL_NONE,
	RPM_REGULATOR_LEVEL_SVS,
	RPM_REGULATOR_LEVEL_NOM,
	RPM_REGULATOR_LEVEL_TURBO,
};

static DEFINE_VDD_REGULATORS(vdd_apcs_pll, ARRAY_SIZE(msm8953_pll_corner),
			     1, msm8953_pll_corner);
static DEFINE_VDD_REGS_INIT(vdd_apcs_cpu, 1);
static const struct clk_ops msm8953_mux_ops;

static struct clk_alpha_pll apcs_hf_pll = {
	.regs = msm8953_pll_regs,
	.min_supported_freq = MSM8953_PLL_RATE_MIN,
	.clkr.hw.init = &(struct clk_init_data) {
		.name = "apcs_hf_pll",
		.parent_names = (const char *[]){ "bi_tcxo_ao" },
		.num_parents = 1,
		.ops = &clk_alpha_pll_ops,
		.vdd_class = &vdd_apcs_pll,
		.rate_max = (unsigned long[ARRAY_SIZE(msm8953_pll_corner)]) {
			[1] = MSM8953_PLL_RATE_MAX,
		},
		.num_rate_max = ARRAY_SIZE(msm8953_pll_corner),
	},
};

static struct clk_regmap_mux_div apcs_mux_c0_clk = {
	.hid_width = 5,
	.src_width = 3,
	.src_shift = 8,
	.safe_src = 4,
	.safe_div = 1,
	.parent_map = cpu_parent_map,
	.clkr.hw.init = &(struct clk_init_data) {
		.name = "a53_pwr_clk",
		.parent_names = cpu_parent_names,
		.num_parents = ARRAY_SIZE(cpu_parent_names),
		.flags = CLK_SET_RATE_PARENT,
		.vdd_class = &vdd_apcs_cpu,
		.ops = &msm8953_mux_ops,
	},
};

static struct clk_regmap_mux_div apcs_mux_c1_clk = {
	.hid_width = 5,
	.src_width = 3,
	.src_shift = 8,
	.safe_src = 4,
	.safe_div = 1,
	.parent_map = cpu_parent_map,
	.clkr.hw.init = &(struct clk_init_data) {
		.name = "a53_perf_clk",
		.parent_names = cpu_parent_names,
		.num_parents = ARRAY_SIZE(cpu_parent_names),
		.flags = CLK_SET_RATE_PARENT,
		.vdd_class = &vdd_apcs_cpu,
		.ops = &msm8953_mux_ops,
	},
};

static struct clk_regmap_mux_div apcs_mux_cci_clk = {
	.hid_width = 5,
	.src_width = 3,
	.src_shift = 8,
	.parent_map = cci_parent_map,
	.clkr.hw.init = &(struct clk_init_data) {
		.name = "cci_clk",
		.parent_names = cci_parent_names,
		.num_parents = ARRAY_SIZE(cci_parent_names),
		.flags = CLK_SET_RATE_PARENT,
		.vdd_class = &vdd_apcs_cpu,
		.ops = &msm8953_mux_ops,
	},
};

static int msm8953_mux_determine_rate(struct clk_hw *hw,
				      struct clk_rate_request *req)
{
	struct clk_regmap_mux_div *mux =
		container_of(to_clk_regmap(hw), struct clk_regmap_mux_div, clkr);
	unsigned long best_delta = ULONG_MAX;
	unsigned long request = req->rate;
	unsigned int i, div;
	bool found = false;

	for (i = 0; i < clk_hw_get_num_parents(hw); i++) {
		struct clk_hw *parent = clk_hw_get_parent_by_index(hw, i);
		unsigned long max_div = BIT(mux->hid_width);

		for (div = 1; div < max_div; div++) {
			long rounded_rate;
			unsigned long parent_rate, actual_rate, delta;

			parent_rate = mult_frac(request, div + 1, 2);
			rounded_rate = clk_hw_round_rate(parent, parent_rate);
			if (rounded_rate <= 0)
				continue;
			parent_rate = rounded_rate;

			actual_rate = mult_frac(parent_rate, 2, div + 1);
			delta = actual_rate > request ? actual_rate - request :
				request - actual_rate;
			if (delta >= best_delta)
				continue;

			best_delta = delta;
			req->rate = actual_rate;
			req->best_parent_hw = parent;
			req->best_parent_rate = parent_rate;
			mux->src = mux->parent_map[i].cfg;
			mux->div = div;
			found = true;

			if (!delta)
				break;
		}
	}

	return found ? 0 : -EINVAL;
}

static int msm8953_mux_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct clk_regmap_mux_div *mux =
		container_of(to_clk_regmap(hw), struct clk_regmap_mux_div, clkr);

	return mux_div_set_src_div(mux, mux->src, mux->div);
}

static int msm8953_mux_set_rate_parent(struct clk_hw *hw, unsigned long rate,
				       unsigned long parent_rate, u8 index)
{
	struct clk_regmap_mux_div *mux =
		container_of(to_clk_regmap(hw), struct clk_regmap_mux_div, clkr);

	return mux_div_set_src_div(mux, mux->parent_map[index].cfg, mux->div);
}

static int msm8953_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_regmap_mux_div *mux =
		container_of(to_clk_regmap(hw), struct clk_regmap_mux_div, clkr);

	return mux_div_set_src_div(mux, mux->parent_map[index].cfg, mux->div);
}

static int msm8953_mux_enable(struct clk_hw *hw)
{
	return clk_regmap_mux_div_ops.enable(hw);
}

static void msm8953_mux_disable(struct clk_hw *hw)
{
	clk_regmap_mux_div_ops.disable(hw);
}

static u8 msm8953_mux_get_parent(struct clk_hw *hw)
{
	return clk_regmap_mux_div_ops.get_parent(hw);
}

static unsigned long msm8953_mux_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	return clk_regmap_mux_div_ops.recalc_rate(hw, parent_rate);
}

static const struct clk_ops msm8953_mux_ops = {
	.enable = msm8953_mux_enable,
	.disable = msm8953_mux_disable,
	.get_parent = msm8953_mux_get_parent,
	.set_parent = msm8953_mux_set_parent,
	.set_rate = msm8953_mux_set_rate,
	.set_rate_and_parent = msm8953_mux_set_rate_parent,
	.determine_rate = msm8953_mux_determine_rate,
	.recalc_rate = msm8953_mux_recalc_rate,
};

static int msm8953_cpu_rate_notifier(struct notifier_block *nb,
				     unsigned long event, void *data)
{
	struct clk_regmap_mux_div *mux =
		container_of(nb, struct clk_regmap_mux_div, clk_nb);

	if (event == PRE_RATE_CHANGE)
		return notifier_from_errno(mux_div_set_src_div(mux,
							mux->safe_src,
							mux->safe_div));

	if (event == ABORT_RATE_CHANGE)
		pr_err("CPU PLL rate change aborted; CPU remains on safe source\n");

	return NOTIFY_OK;
}

static const struct regmap_config pll_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = 0x34,
	.fast_io = true,
};

static const struct regmap_config rcg_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = 0x4,
	.fast_io = true,
};

static int msm8953_read_speed_bin(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *base;
	u32 fuse;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "efuse");
	if (!res) {
		dev_info(&pdev->dev, "No speed bin fuse; using bin 0\n");
		return 0;
	}

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return dev_err_probe(&pdev->dev, PTR_ERR(base),
				     "Failed to map speed bin fuse\n");

	fuse = readl_relaxed(base);
	return (fuse >> 8) & 0x7;
}

static int msm8953_load_rate_table(struct platform_device *pdev,
				   const char *property,
				   struct clk_init_data *init,
				   bool load_voltage)
{
	struct device *dev = &pdev->dev;
	int count, i, ret;
	u32 *values;

	count = of_property_count_u32_elems(dev->of_node, property);
	if (count < 0)
		return dev_err_probe(dev, count, "Missing %s\n", property);
	if (!count || count % 2)
		return dev_err_probe(dev, -EINVAL, "Invalid %s table\n",
				     property);

	count /= 2;
	if (!load_voltage && count != vdd_apcs_cpu.num_levels)
		return dev_err_probe(dev, -EINVAL,
				     "Inconsistent %s table length\n", property);
	values = devm_kcalloc(dev, count * 2, sizeof(*values), GFP_KERNEL);
	init->rate_max = devm_kcalloc(dev, count, sizeof(*init->rate_max),
				      GFP_KERNEL);
	if (!values || !init->rate_max)
		return -ENOMEM;

	ret = of_property_read_u32_array(dev->of_node, property, values,
					 count * 2);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to read %s\n", property);

	if (load_voltage) {
		vdd_apcs_cpu.vdd_uv = devm_kcalloc(dev, count,
						  sizeof(*vdd_apcs_cpu.vdd_uv),
						  GFP_KERNEL);
		vdd_apcs_cpu.level_votes = devm_kcalloc(dev, count,
					 sizeof(*vdd_apcs_cpu.level_votes),
					 GFP_KERNEL);
		if (!vdd_apcs_cpu.vdd_uv || !vdd_apcs_cpu.level_votes)
			return -ENOMEM;
		vdd_apcs_cpu.num_levels = count;
		vdd_apcs_cpu.cur_level = count;
	}

	for (i = 0; i < count; i++) {
		init->rate_max[i] = values[i * 2];
		if (load_voltage)
			vdd_apcs_cpu.vdd_uv[i] = values[i * 2 + 1];
		else if (vdd_apcs_cpu.vdd_uv[i] != values[i * 2 + 1])
			return dev_err_probe(dev, -EINVAL,
					     "Inconsistent CCI voltage table\n");
	}
	init->num_rate_max = count;

	return 0;
}

static int msm8953_map_regmap(struct platform_device *pdev,
			      const char *name,
			      const struct regmap_config *config,
			      struct regmap **regmap)
{
	struct resource *res;
	void __iomem *base;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (!res)
		return dev_err_probe(&pdev->dev, -EINVAL,
				     "Missing %s register range\n", name);

	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return dev_err_probe(&pdev->dev, PTR_ERR(base),
				     "Failed to map %s\n", name);

	*regmap = devm_regmap_init_mmio(&pdev->dev, base, config);
	if (IS_ERR(*regmap))
		return dev_err_probe(&pdev->dev, PTR_ERR(*regmap),
				     "Failed to create %s regmap\n", name);

	return 0;
}

static int msm8953_register_clock(struct device *dev, struct clk_hw *hw)
{
	int ret = devm_clk_hw_register(dev, hw);

	if (ret)
		dev_err(dev, "Failed to register clock %s: %d\n",
			hw->init->name, ret);
	return ret;
}

static int msm8953_cpu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct clk_hw_onecell_data *data;
	struct clk *xo;
	struct regulator *vdd_mx, *vdd_cl;
	u32 pll_l;
	struct alpha_pll_config pll_config = {
		.l = 0x44,
		.main_output_mask = BIT(0),
		.early_output_mask = BIT(3),
		.pre_div_mask = BIT(12),
		.post_div_val = 0x100,
		.post_div_mask = 0x300,
		.config_ctl_val = 0x200D4828,
		.config_ctl_hi_val = 0x6,
		.test_ctl_val = 0x1C000000,
		.test_ctl_hi_val = 0x00004000,
		.test_ctl_mask = 0xffffffff,
		.test_ctl_hi_mask = 0xffffffff,
	};
	struct regmap *regmap;
	char property[40];
	int bin, ret, cpu;

	bin = msm8953_read_speed_bin(pdev);
	if (bin < 0)
		return bin;

	snprintf(property, sizeof(property), "qcom,speed%d-bin-v0-cl", bin);
	if (!of_find_property(dev->of_node, property, NULL)) {
		dev_warn(dev, "Missing %s; using speed0 table\n", property);
		snprintf(property, sizeof(property), "qcom,speed0-bin-v0-cl");
	}
	ret = msm8953_load_rate_table(pdev, property,
			(struct clk_init_data *)apcs_mux_c0_clk.clkr.hw.init, true);
	if (ret)
		return ret;
	((struct clk_init_data *)apcs_mux_c1_clk.clkr.hw.init)->rate_max =
		apcs_mux_c0_clk.clkr.hw.init->rate_max;
	((struct clk_init_data *)apcs_mux_c1_clk.clkr.hw.init)->num_rate_max =
		apcs_mux_c0_clk.clkr.hw.init->num_rate_max;

	snprintf(property, sizeof(property), "qcom,speed%d-bin-v0-cci", bin);
	if (!of_find_property(dev->of_node, property, NULL)) {
		dev_warn(dev, "Missing %s; using speed0 table\n", property);
		snprintf(property, sizeof(property), "qcom,speed0-bin-v0-cci");
	}
	ret = msm8953_load_rate_table(pdev, property,
			(struct clk_init_data *)apcs_mux_cci_clk.clkr.hw.init,
			false);
	if (ret)
		return ret;

	vdd_mx = devm_regulator_get(dev, "vdd-mx");
	if (IS_ERR(vdd_mx))
		return dev_err_probe(dev, PTR_ERR(vdd_mx),
				     "Failed to get vdd-mx regulator\n");
	vdd_cl = devm_regulator_get(dev, "vdd-cl");
	if (IS_ERR(vdd_cl))
		return dev_err_probe(dev, PTR_ERR(vdd_cl),
				     "Failed to get vdd-cl regulator\n");
	vdd_apcs_pll.regulator[0] = vdd_mx;
	vdd_apcs_cpu.regulator[0] = vdd_cl;

	ret = msm8953_map_regmap(pdev, "c0-pll", &pll_regmap_config,
				 &regmap);
	if (ret)
		return ret;
	apcs_hf_pll.clkr.regmap = regmap;
	ret = regmap_read(regmap, msm8953_pll_regs[PLL_OFF_L_VAL], &pll_l);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to read CPU PLL L value\n");
	if (!pll_l) {
		clk_alpha_pll_configure(&apcs_hf_pll, regmap, &pll_config);
		ret = regmap_update_bits(regmap,
					 msm8953_pll_regs[PLL_OFF_USER_CTL],
					 BIT(24), 0);
		if (ret)
			return dev_err_probe(dev, ret,
					     "Failed to configure CPU PLL\n");
	}

	ret = msm8953_map_regmap(pdev, "c0-mux", &rcg_regmap_config,
				 &apcs_mux_c0_clk.clkr.regmap);
	if (ret)
		return ret;
	ret = msm8953_map_regmap(pdev, "c1-mux", &rcg_regmap_config,
				 &apcs_mux_c1_clk.clkr.regmap);
	if (ret)
		return ret;
	ret = msm8953_map_regmap(pdev, "cci-mux", &rcg_regmap_config,
				 &apcs_mux_cci_clk.clkr.regmap);
	if (ret)
		return ret;

	xo = devm_clk_get(dev, "xo_a");
	if (IS_ERR(xo))
		return dev_err_probe(dev, PTR_ERR(xo),
				     "Failed to get XO clock\n");

	ret = msm8953_register_clock(dev, &apcs_hf_pll.clkr.hw);
	if (ret)
		return ret;
	ret = msm8953_register_clock(dev, &apcs_mux_c0_clk.clkr.hw);
	if (ret)
		return ret;
	ret = msm8953_register_clock(dev, &apcs_mux_c1_clk.clkr.hw);
	if (ret)
		return ret;
	ret = msm8953_register_clock(dev, &apcs_mux_cci_clk.clkr.hw);
	if (ret)
		return ret;

	apcs_mux_c0_clk.clk_nb.notifier_call = msm8953_cpu_rate_notifier;
	apcs_mux_c1_clk.clk_nb.notifier_call = msm8953_cpu_rate_notifier;
	ret = clk_notifier_register(apcs_mux_c0_clk.clkr.hw.clk,
				    &apcs_mux_c0_clk.clk_nb);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to register C0 notifier\n");
	ret = clk_notifier_register(apcs_mux_c1_clk.clkr.hw.clk,
				    &apcs_mux_c1_clk.clk_nb);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to register C1 notifier\n");

	data = devm_kzalloc(dev, struct_size(data, hws, CPU_MSM8953_NUM_CLKS),
			    GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	data->num = CPU_MSM8953_NUM_CLKS;
	data->hws[CPU_MSM8953_CCI_CLK] = &apcs_mux_cci_clk.clkr.hw;
	data->hws[CPU_MSM8953_PWR_CLK] = &apcs_mux_c0_clk.clkr.hw;
	data->hws[CPU_MSM8953_PERF_CLK] = &apcs_mux_c1_clk.clkr.hw;
	ret = of_clk_add_hw_provider(dev->of_node, of_clk_hw_onecell_get, data);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to register CPU clocks\n");

	ret = clk_set_rate(apcs_mux_c0_clk.clkr.hw.clk, MSM8953_PLL_RATE_MIN);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to set initial CPU rate\n");
	ret = clk_set_rate(apcs_mux_c1_clk.clkr.hw.clk, MSM8953_PLL_RATE_MIN);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to set initial CPU rate\n");
	ret = clk_set_rate(apcs_mux_cci_clk.clkr.hw.clk,
			   mult_frac(MSM8953_PLL_RATE_MIN, 2,
				     MSM8953_CCI_DIVIDER));
	if (ret)
		return dev_err_probe(dev, ret, "Failed to set initial CCI rate\n");

	ret = clk_prepare_enable(apcs_mux_c0_clk.clkr.hw.clk);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to enable C0 clock\n");
	ret = clk_prepare_enable(apcs_mux_c1_clk.clkr.hw.clk);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to enable C1 clock\n");
	ret = clk_prepare_enable(apcs_mux_cci_clk.clkr.hw.clk);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to enable CCI clock\n");

	ret = clk_set_rate(apcs_mux_c0_clk.clkr.hw.clk, MSM8953_BOOT_RATE);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to set boot CPU rate\n");

	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);
		int i;

		if (!cpu_dev)
			return dev_err_probe(dev, -ENODEV,
					     "CPU%d device is unavailable\n", cpu);

		for (i = 1; i < apcs_mux_c0_clk.clkr.hw.init->num_rate_max; i++) {
			ret = dev_pm_opp_add(cpu_dev,
				apcs_mux_c0_clk.clkr.hw.init->rate_max[i],
				vdd_apcs_cpu.vdd_uv[i]);
			if (ret && ret != -EEXIST) {
				dev_err(dev, "Failed to add CPU%d OPP %lu: %d\n",
					cpu,
					apcs_mux_c0_clk.clkr.hw.init->rate_max[i],
					ret);
				return ret;
			}
		}
	}

	ret = of_platform_populate(dev->of_node, NULL, NULL, dev);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to populate CPU children\n");

	dev_info(dev, "MSM8953 CCF CPU/CCI clocks registered\n");
	return 0;
}

static const struct of_device_id msm8953_cpu_match[] = {
	{ .compatible = "qcom,cpu-clock-8953" },
	{}
};

static struct platform_driver msm8953_cpu_driver = {
	.probe = msm8953_cpu_probe,
	.driver = {
		.name = "qcom-cpu-msm8953",
		.of_match_table = msm8953_cpu_match,
	},
};
builtin_platform_driver(msm8953_cpu_driver);
