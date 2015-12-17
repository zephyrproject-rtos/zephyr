ifeq ($(ARCH),arc)
PLATFORM_CONFIG ?= quark_se_ss
else
PLATFORM_CONFIG ?= arduino_101
endif

KERNEL_TYPE ?= nano
CONF_FILE = prj_$(ARCH).conf

include ${ZEPHYR_BASE}/Makefile.inc
