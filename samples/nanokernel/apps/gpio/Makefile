ifeq ($(ARCH),arc)
BOARD ?= arduino_101_sss
else
ifeq ($(ARCH),arm)
BOARD ?= arduino_due
else
BOARD ?= arduino_101
endif
endif

KERNEL_TYPE ?= nano
CONF_FILE = prj_$(ARCH).conf

include ${ZEPHYR_BASE}/Makefile.inc
