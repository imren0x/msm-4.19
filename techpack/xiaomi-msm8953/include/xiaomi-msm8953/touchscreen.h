#ifndef _XIAOMI_MSM8953_TOUCHSCREEN_H
#define _XIAOMI_MSM8953_TOUCHSCREEN_H

#if IS_ENABLED(CONFIG_TOUCHSCREEN_SYSCTL_MI8953)
#include <linux/device.h>
 
struct xiaomi_msm8953_touchscreen_operations_t {
 	struct device *dev;
 	int (*enable_dt2w)(struct device *dev, bool enable);
 	int (*disable_keys)(struct device *dev, bool disable);
};
 
extern int xiaomi_msm8953_touchscreen_register_operations(struct xiaomi_msm8953_touchscreen_operations_t *ts_ops);
#endif
 
#endif