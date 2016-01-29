ifeq ($(ARCH),arc)
BOARD ?= arduino_101_sss
else
BOARD ?= arduino_101
endif

KERNEL_TYPE ?= nano
CONF_FILE = prj_$(ARCH).conf

include ${ZEPHYR_BASE}/Makefile.inc
