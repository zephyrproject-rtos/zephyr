MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
BOARD ?= qemu_x86
CONF_FILE = prj.conf

ifeq (${KERNEL_TYPE},unified)
$(error This project will only run a unified kernel when round-robin scheduling is implemented)
endif

include ${ZEPHYR_BASE}/Makefile.inc
