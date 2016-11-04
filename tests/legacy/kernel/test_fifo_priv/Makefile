MDEF_FILE = prj.mdef
BOARD ?= qemu_x86
CONF_FILE = prj.conf
SOURCE_DIR := $(ZEPHYR_BASE)/tests/legacy/kernel/test_fifo/microkernel/src
# Enable testing for private microkernel FIFOs
CFLAGS = -DTEST_PRIV_FIFO

include ${ZEPHYR_BASE}/Makefile.inc
