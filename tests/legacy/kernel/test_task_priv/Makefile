MDEF_FILE = prj.mdef
BOARD ?= qemu_x86
CONF_FILE = prj.conf
SOURCE_DIR := $(ZEPHYR_BASE)/tests/legacy/kernel/test_task/src

# Enable testing for private microkernel task objects
CFLAGS = -DTEST_PRIV_TASKS

include ${ZEPHYR_BASE}/Makefile.inc
