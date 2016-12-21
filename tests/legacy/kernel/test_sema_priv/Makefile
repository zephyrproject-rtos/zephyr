MDEF_FILE = prj.mdef
BOARD ?= qemu_x86
CONF_FILE = prj.conf
SOURCE_DIR := $(ZEPHYR_BASE)/tests/legacy/kernel/test_sema/microkernel/src

# Enable testing of private microkernel semaphore
CFLAGS = -DTEST_PRIV_KSEM

include ${ZEPHYR_BASE}/Makefile.test
