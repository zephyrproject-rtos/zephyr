MDEF_FILE = prj.mdef
BOARD ?= qemu_x86
CONF_FILE = prj.conf
SOURCE_DIR := $(ZEPHYR_BASE)/tests/legacy/kernel/test_mail/src

# Enable testing for private microkernel mailboxes
CFLAGS = -DTEST_PRIV_MBX

include ${ZEPHYR_BASE}/Makefile.inc
