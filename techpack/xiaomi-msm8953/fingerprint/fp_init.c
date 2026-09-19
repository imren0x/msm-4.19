#include <linux/export.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <xiaomi-msm8953/mach.h>

#if IS_ENABLED(CONFIG_MACH_XIAOMI_YSL)
enum xiaomi_ysl_fpsensor_variants {
	XIAOMI_YSL_FPSENSOR_UNKNOWN = 0,
	XIAOMI_YSL_FPSENSOR_FPC,
	XIAOMI_YSL_FPSENSOR_GOODIX,
};

static enum xiaomi_ysl_fpsensor_variants xiaomi_ysl_fpsensor_variant = XIAOMI_YSL_FPSENSOR_UNKNOWN;

static int __init xiaomi_ysl_fpsensor_setup(char *str)
{
	if (strcmp(str, "fpc") == 0)
		xiaomi_ysl_fpsensor_variant = XIAOMI_YSL_FPSENSOR_FPC;
	else if (strcmp(str, "goodix") == 0 || strcmp(str, "gdx") == 0)
		xiaomi_ysl_fpsensor_variant = XIAOMI_YSL_FPSENSOR_GOODIX;

	pr_info("%s: Xiaomi YSL fingerprint sensor variant: %s %d\n", __func__, str, xiaomi_ysl_fpsensor_variant);
	return 1;
}
__setup("androidboot.fpsensor=", xiaomi_ysl_fpsensor_setup);
#endif

extern int xiaomi_msm8953_fingerprint_fpc_mido_init(void);
extern int xiaomi_msm8953_fingerprint_fpc_rosy_init(void);
extern int xiaomi_msm8953_fingerprint_fpc_ysl_init(void);
extern int xiaomi_msm8953_fingerprint_goodix_mido_init(void);
extern int xiaomi_msm8953_fingerprint_goodix_rosy_init(void);
extern int xiaomi_msm8953_fingerprint_goodix_vince_init(void);
extern int xiaomi_msm8953_fingerprint_goodix_ysl_init(void);

static int __init xiaomi_msm8953_fingerprint_init(void)
{
#if IS_ENABLED(CONFIG_MACH_XIAOMI_YSL)
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_YSL) {
		switch (xiaomi_ysl_fpsensor_variant) {
			case XIAOMI_YSL_FPSENSOR_FPC:
				xiaomi_msm8953_fingerprint_fpc_ysl_init();
				return 0;
			case XIAOMI_YSL_FPSENSOR_GOODIX:
				xiaomi_msm8953_fingerprint_goodix_ysl_init();
				return 0;
			default:
				xiaomi_msm8953_fingerprint_fpc_ysl_init();
				xiaomi_msm8953_fingerprint_goodix_ysl_init();
				return 0;
		}
	}
#endif
	
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_VINCE ||
		xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_MIDO)
		xiaomi_msm8953_fingerprint_fpc_mido_init();
	
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_ROSY)
		xiaomi_msm8953_fingerprint_fpc_rosy_init();
	
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_MIDO)
		xiaomi_msm8953_fingerprint_goodix_mido_init();
	
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_ROSY)
		xiaomi_msm8953_fingerprint_goodix_rosy_init();
	
	if (xiaomi_msm8953_mach_get() == XIAOMI_MSM8953_MACH_VINCE)
		xiaomi_msm8953_fingerprint_goodix_vince_init();

	return 0;
}
module_init(xiaomi_msm8953_fingerprint_init);
