MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
BOARD ?= qemu_x86
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR := $(ZEPHYR_BASE)/samples/microkernel/test/test_pipe/src

# Enable testing for private microkernel pipe objects
CFLAGS = -DTEST_PRIV_PIPES

include ${ZEPHYR_BASE}/Makefile.inc
