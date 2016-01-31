MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
BOARD ?= qemu_x86
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR := $(ZEPHYR_BASE)/tests/kernel/test_fifo/src
# Enable testing for private microkernel FIFOs
CFLAGS = -DTEST_PRIV_FIFO

include ${ZEPHYR_BASE}/Makefile.inc
