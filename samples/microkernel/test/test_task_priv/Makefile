MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
BOARD ?= qemu_x86
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR := $(ZEPHYR_BASE)/samples/microkernel/test/test_task/src

# Enable testing for private microkernel task objects
CFLAGS = -DTEST_PRIV_TASKS

include ${ZEPHYR_BASE}/Makefile.inc
