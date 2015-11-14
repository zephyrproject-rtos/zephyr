MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= qemu_x86
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR := $(ZEPHYR_BASE)/samples/microkernel/test/test_sema/src

# Enable testing of private microkernel semaphore
CFLAGS = -DTEST_PRIV_KSEM

include ${ZEPHYR_BASE}/Makefile.inc
