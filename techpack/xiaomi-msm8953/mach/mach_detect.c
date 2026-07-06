#include <linux/export.h>
#include <linux/kobject.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <xiaomi-msm8953/mach.h>

typedef struct xiaomi_msm8953_mach_info {
	enum xiaomi_msm8953_mach_family_types mach_family;
	const char *of_compatible;
	const char *codename;
	const char *product_name;
} xiaomi_msm8953_mach_info_t;

static const xiaomi_msm8953_mach_info_t xiaomi_msm8953_mach_table[XIAOMI_MSM8953_MACH_MAX] = {
    // No Family
	[XIAOMI_MSM8953_MACH_UTER] = {XIAOMI_MSM8953_MACH_FAMILY_UNKNOWN, "xiaomi,uter", "uter", "Redmi Note 5A (Prototype)"},

	// Family MTP (MSM8953)
	[XIAOMI_MSM8953_MACH_YSL] = {XIAOMI_MSM8953_MACH_FAMILY_MTP, "xiaomi,ysl", "ysl", "Redmi S2 / Y2"},

	// Family QRD (SDM450)
	[XIAOMI_MSM8953_MACH_ROSY] = {XIAOMI_MSM8953_MACH_FAMILY_QRD, "xiaomi,rosy", "rosy", "Redmi 5"},

	// Family QRD (MSM8953)
	[XIAOMI_MSM8953_MACH_VINCE] = {XIAOMI_MSM8953_MACH_FAMILY_QRD, "xiaomi,vince", "vince", "Redmi 5 Plus"},

	[XIAOMI_MSM8953_MACH_TISSOT] = {XIAOMI_MSM8953_MACH_FAMILY_QRD, "xiaomi,tissot", "tissot", "Mi A1"},

	[XIAOMI_MSM8953_MACH_TIFFANY] = {XIAOMI_MSM8953_MACH_FAMILY_QRD, "xiaomi,tiffany", "tiffany", "Mi 5X"},

	[XIAOMI_MSM8953_MACH_DAISY] = {XIAOMI_MSM8953_MACH_FAMILY_QRD, "xiaomi,daisy", "daisy", "Mi A1"},

	[XIAOMI_MSM8953_MACH_MIDO] = {XIAOMI_MSM8953_MACH_FAMILY_QRD, "xiaomi,mido", "mido", "Redmi Note 4 / 4x"},

	[XIAOMI_MSM8953_MACH_MARKW] = {XIAOMI_MSM8953_MACH_FAMILY_QRD, "xiaomi,markw", "markw", "Redmi 4 Prime"},
};

static enum xiaomi_msm8953_mach_types saved_mach = XIAOMI_MSM8953_MACH_UNKNOWN;
static struct kobject *xiaomi_msm8953_mach_kobj;

enum xiaomi_msm8953_mach_family_types xiaomi_msm8953_mach_get_family(void) {
	if (!saved_mach)
		return XIAOMI_MSM8953_MACH_FAMILY_UNKNOWN;
	return xiaomi_msm8953_mach_table[saved_mach].mach_family;
}
EXPORT_SYMBOL(xiaomi_msm8953_mach_get_family);

enum xiaomi_msm8953_mach_types xiaomi_msm8953_mach_get(void) {
	return saved_mach;
}
EXPORT_SYMBOL(xiaomi_msm8953_mach_get);

static ssize_t xiaomi_msm8953_mach_codename_show(struct kobject *kobj,
						struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", xiaomi_msm8953_mach_table[saved_mach].codename);
}

static struct kobj_attribute xiaomi_msm8953_mach_codename_attr = {
	.attr = {
		.name = "codename",
		.mode = 0444,
	},
	.show = xiaomi_msm8953_mach_codename_show,
};

static ssize_t xiaomi_msm8953_mach_product_name_show(struct kobject *kobj,
						struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", xiaomi_msm8953_mach_table[saved_mach].product_name);
}

static struct kobj_attribute xiaomi_msm8953_mach_product_name_attr = {
	.attr = {
		.name = "product_name",
		.mode = 0444,
	},
	.show = xiaomi_msm8953_mach_product_name_show,
};

static ssize_t xiaomi_msm8953_mach_supported_machines_show(struct kobject *kobj,
							struct kobj_attribute *attr, char *buf)
{
	int i, count = 0;

	for (i=1; i<XIAOMI_MSM8953_MACH_MAX; ++i) {
		count += snprintf(buf+count, PAGE_SIZE - count, "%s (%s)\n",
				xiaomi_msm8953_mach_table[i].product_name,
				xiaomi_msm8953_mach_table[i].codename);
	}

	return count;
}

static struct kobj_attribute xiaomi_msm8953_mach_supported_machines_attr = {
	.attr = {
		.name = "supported_machines",
		.mode = 0444,
	},
	.show = xiaomi_msm8953_mach_supported_machines_show,
};

static int xiaomi_msm8953_mach_detect(void) {
	int i, rc;

	for (i=1; i<XIAOMI_MSM8953_MACH_MAX; ++i) {
		rc = of_machine_is_compatible(xiaomi_msm8953_mach_table[i].of_compatible);
		if (rc) {
			saved_mach = i;
			break;
		}
	}

	return rc;
}

static int __init xiaomi_msm8953_mach_detect_init(void) {
	int rc;

	// Detect
	rc = xiaomi_msm8953_mach_detect();
	if (!rc) {
		pr_err("%s: Could not detect Xiaomi MSM8953 machine, rc=%d\n", __func__, rc);
		goto fail;
	}

	// Print the current machine
	pr_info("%s: Current machine: %s (%s)\n", __func__,
			xiaomi_msm8953_mach_table[saved_mach].product_name,
			xiaomi_msm8953_mach_table[saved_mach].codename);

	// Create sysfs dir
	xiaomi_msm8953_mach_kobj = kobject_create_and_add("xiaomi-msm8953-mach", NULL);
	if (!xiaomi_msm8953_mach_kobj) {
		pr_err("%s: Failed to create sysfs dir\n", __func__);
		goto fail;
	}

	// Create sysfs files
	rc = sysfs_create_file(xiaomi_msm8953_mach_kobj,
			&xiaomi_msm8953_mach_codename_attr.attr);
	if (rc < 0)
		pr_err("%s: Failed to create sysfs file codename, rc=%d\n", __func__, rc);

	rc = sysfs_create_file(xiaomi_msm8953_mach_kobj,
			&xiaomi_msm8953_mach_product_name_attr.attr);
	if (rc < 0)
		pr_err("%s: Failed to create sysfs file product_name, rc=%d\n", __func__, rc);

	rc = sysfs_create_file(xiaomi_msm8953_mach_kobj,
			&xiaomi_msm8953_mach_supported_machines_attr.attr);
	if (rc < 0)
		pr_err("%s: Failed to create sysfs file supported_machines, rc=%d\n", __func__, rc);
	
	return 0;
fail:
	return rc;
}
core_initcall(xiaomi_msm8953_mach_detect_init);