ifeq ($(ARCH),arc)
BOARD ?= quark_se_ss
else
BOARD ?= arduino_101
endif

KERNEL_TYPE ?= nano
CONF_FILE = prj_$(ARCH).conf

include ${ZEPHYR_BASE}/Makefile.inc
