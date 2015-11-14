MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= qemu_x86
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR := $(ZEPHYR_BASE)/samples/microkernel/test/test_mail/src

# Enable testing for private microkernel mailboxes
CFLAGS = -DTEST_PRIV_MBX

include ${ZEPHYR_BASE}/Makefile.inc
