#include <linux/module.h>
#include <linux/printk.h>
#if IS_ENABLED(CONFIG_MACH_XIAOMI_MSM8953)
#include <xiaomi-msm8953/mach.h>
#endif

/*
extern bool camera_legacy_enable;

	if (!camera_legacy_enable)
		return -ENODEV;
*/

bool camera_legacy_enable = false;

static int __init camera_legacy_init(void)
{
#if IS_ENABLED(CONFIG_MACH_XIAOMI_MSM8953)
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_VINCE ||
		xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_DAISY ||
		xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_SAKURA ||
		xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_TIFFANY ||
		xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_ROSY ||
		xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_YSL)
		camera_legacy_enable = true;
#endif

#if IS_ENABLED(CONFIG_MSMB_CAMERA_LEGACY_M) || \
    IS_ENABLED(CONFIG_MSMB_CAMERA_LEGACY_N)
#if IS_ENABLED(CONFIG_MACH_XIAOMI_MIDO) || IS_ENABLED(CONFIG_MACH_XIAOMI_MARKW)
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_MIDO ||
		xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_MARKW)
		camera_legacy_enable = false;
#endif
#endif

	if (camera_legacy_enable)
		pr_info("Enable Legacy Camera Stack.\n");

	return 0;
}

module_init(camera_legacy_init);
