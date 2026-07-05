ifeq ($(CONFIG_MACH_XIAOMI_MSM8953),y)
obj-$(CONFIG_MACH_XIAOMI_MSM8953) += mach/
obj-$(CONFIG_INPUT_TOUCHSCREEN) += touchscreen/
obj-y += pmic/
obj-y += power_supply/
obj-y += vibrator/
obj-y += fingerprint/
obj-y += ir/
obj-y += leds/

else
ccflags-y := -Wno-unused-function
obj-y := stub.o
endif
