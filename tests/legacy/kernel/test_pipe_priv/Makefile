MDEF_FILE = prj.mdef
BOARD ?= qemu_x86
CONF_FILE = prj.conf
SOURCE_DIR := $(ZEPHYR_BASE)/tests/legacy/kernel/test_pipe/src

# Enable testing for private microkernel pipe objects
CFLAGS = -DTEST_PRIV_PIPES

include ${ZEPHYR_BASE}/Makefile.test
