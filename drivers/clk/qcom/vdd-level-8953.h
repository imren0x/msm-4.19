/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __DRIVERS_CLK_QCOM_VDD_LEVEL_8953_H
#define __DRIVERS_CLK_QCOM_VDD_LEVEL_8953_H

#include <linux/regulator/rpm-smd-regulator.h>

#define VDD_DIG_FMAX_MAP4(l1, f1, l2, f2, l3, f3, l4, f4) \
	.vdd_class = &vdd_dig, \
	.rate_max = (unsigned long[VDD_DIG_NUM]) { \
		[VDD_DIG_##l1] = (f1), \
		[VDD_DIG_##l2] = (f2), \
		[VDD_DIG_##l3] = (f3), \
		[VDD_DIG_##l4] = (f4), \
	}, \
	.num_rate_max = VDD_DIG_NUM

#define VDD_DIG_FMAX_MAP5(l1, f1, l2, f2, l3, f3, l4, f4, l5, f5) \
	.vdd_class = &vdd_dig, \
	.rate_max = (unsigned long[VDD_DIG_NUM]) { \
		[VDD_DIG_##l1] = (f1), \
		[VDD_DIG_##l2] = (f2), \
		[VDD_DIG_##l3] = (f3), \
		[VDD_DIG_##l4] = (f4), \
		[VDD_DIG_##l5] = (f5), \
	}, \
	.num_rate_max = VDD_DIG_NUM

enum vdd_dig_levels {
	VDD_DIG_NONE,
	VDD_DIG_MIN_SVS,
	VDD_DIG_LOW_SVS,
	VDD_DIG_SVS,
	VDD_DIG_SVS_PLUS,
	VDD_DIG_NOM,
	VDD_DIG_NOM_PLUS,
	VDD_DIG_HIGH,
	VDD_DIG_NUM,
};

static int vdd_corner[] = {
	RPM_REGULATOR_LEVEL_NONE,
	RPM_REGULATOR_LEVEL_MIN_SVS,
	RPM_REGULATOR_LEVEL_LOW_SVS,
	RPM_REGULATOR_LEVEL_SVS,
	RPM_REGULATOR_LEVEL_SVS_PLUS,
	RPM_REGULATOR_LEVEL_NOM,
	RPM_REGULATOR_LEVEL_NOM_PLUS,
	RPM_REGULATOR_LEVEL_TURBO,
};

#endif
