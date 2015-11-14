MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= qemu_x86
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR := $(ZEPHYR_BASE)/samples/microkernel/test/test_fifo/src
# Enable testing for private microkernel FIFOs
CFLAGS = -DTEST_PRIV_FIFO

include ${ZEPHYR_BASE}/Makefile.inc
