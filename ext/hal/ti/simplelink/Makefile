ifdef CONFIG_HAS_CC3220SDK
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/ti/simplelink/source/ti/devices/cc32xx
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/ti/simplelink/source/ti/devices/cc32xx/inc
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/ti/simplelink/source/ti/devices/cc32xx/driverlib
KBUILD_CFLAGS += -DUSE_CC3220_ROM_DRV_API
endif # CONFIG_HAS_CC3220SDK
ifdef CONFIG_SIMPLELINK_HOST_DRIVER
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/ti/simplelink/source
KBUILD_CFLAGS += -DSL_SUPPORT_IPV6
KBUILD_CFLAGS += -DSL_PLATFORM_MULTI_THREADED
endif # CONFIG_SIMPLELINK_HOST_DRIVER
