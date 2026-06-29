#include <linux/module.h>
#include <linux/printk.h>
#if IS_ENABLED(CONFIG_MACH_XIAOMI_MSM8953)
#include <xiaomi-msm8953/mach.h>
#endif

/*
extern bool camera_legacy_n_enable;

	if (!camera_legacy_n_enable)
		return -ENODEV;
*/

bool camera_legacy_n_enable = false;

static int __init camera_legacy_n_init(void)
{

#if IS_ENABLED(CONFIG_MACH_XIAOMI_MIDO)
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_MIDO)
		camera_legacy_n_enable = true;
#endif

	if (camera_legacy_n_enable)
		pr_info("Enable Legacy Camera Stack.\n");

	return 0;
}

module_init(camera_legacy_n_init);
