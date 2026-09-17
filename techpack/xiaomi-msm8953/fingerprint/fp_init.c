#include <linux/export.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <xiaomi-msm8953/mach.h>

extern int xiaomi_msm8953_fingerprint_fpc_mido_init(void);
extern int xiaomi_msm8953_fingerprint_fpc_rosy_init(void);
extern int xiaomi_msm8953_fingerprint_fpc_ysl_init(void);
extern int xiaomi_msm8953_fingerprint_goodix_mido_init(void);
extern int xiaomi_msm8953_fingerprint_goodix_rosy_init(void);
extern int xiaomi_msm8953_fingerprint_goodix_vince_init(void);
extern int xiaomi_msm8953_fingerprint_goodix_ysl_init(void);

static int __init xiaomi_msm8953_fingerprint_init(void)
{
	xiaomi_msm8953_fingerprint_fpc_mido_init();
    xiaomi_msm8953_fingerprint_fpc_rosy_init();
    xiaomi_msm8953_fingerprint_fpc_ysl_init();
	xiaomi_msm8953_fingerprint_goodix_mido_init();
    xiaomi_msm8953_fingerprint_goodix_rosy_init();
    xiaomi_msm8953_fingerprint_goodix_vince_init();
    xiaomi_msm8953_fingerprint_goodix_ysl_init();

	return 0;
}
module_init(xiaomi_msm8953_fingerprint_init);