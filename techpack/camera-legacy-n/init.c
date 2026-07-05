#include <linux/module.h>
#include <linux/printk.h>

/*
extern bool camera_legacy_n_enable;

	if (!camera_legacy_n_enable)
		return -ENODEV;
*/

bool camera_legacy_n_enable = false;

static int __init camera_legacy_n_init(void)
{
	camera_legacy_n_enable = true;

	if (camera_legacy_n_enable)
		pr_info("Enable Legacy Camera Stack.\n");

	return 0;
}

module_init(camera_legacy_n_init);
