# auto-detect subdirs
ifeq ($(CONFIG_ARCH_SDM450), y)
include $(srctree)/techpack/audio-legacy/config/sdm450auto.conf
export
endif
ifeq ($(CONFIG_ARCH_SDM660), y)
include $(srctree)/techpack/audio-legacy/config/sdm660auto.conf
endif
ifeq ($(CONFIG_ARCH_MSM8953), y)
include $(srctree)/techpack/audio-legacy/config/sdm450auto.conf
export
endif
ifeq ($(CONFIG_ARCH_MSM8937), y)
include $(srctree)/techpack/audio-legacy/config/sdm450auto.conf
export
endif

# Use USERINCLUDE when you must reference the UAPI directories only.
USERINCLUDE     += \
                 -I$(srctree)/techpack/audio-legacy/include/uapi \
                -I$(srctree)/techpack/audio-legacy/include

# Use LINUXINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
LINUXINCLUDE    += \
                -I$(srctree)/techpack/audio-legacy/include/uapi \
                -I$(srctree)/techpack/audio-legacy/include

ifeq ($(CONFIG_ARCH_SDM450), y)
LINUXINCLUDE    += \
                -include $(srctree)/techpack/audio-legacy/config/sdm450autoconf.h
endif
ifeq ($(CONFIG_ARCH_SDM660), y)
LINUXINCLUDE    += \
                -include $(srctree)/techpack/audio-legacy/config/sdm660autoconf.h
endif
ifeq ($(CONFIG_ARCH_MSM8953), y)
LINUXINCLUDE    += \
                -include $(srctree)/techpack/audio-legacy/config/sdm450autoconf.h
endif
ifeq ($(CONFIG_ARCH_MSM8937), y)
LINUXINCLUDE    += \
                -include $(srctree)/techpack/audio-legacy/config/sdm450autoconf.h
endif
obj-y += soc/
obj-y += dsp/
obj-y += ipc/
obj-y += asoc/
