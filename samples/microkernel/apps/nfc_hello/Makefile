KERNEL_TYPE ?= nano
KBUILD_DEFCONFIG ?= $(KERNEL_TYPE)_generic_pc_defconfig
CONF_FILE = prj.conf
SOURCE_DIR = $(ZEPHYR_BASE)/samples/microkernel/apps/nfc_hello/src/

QEMU_EXTRA_FLAGS += -serial tcp:localhost:8888

include ${ZEPHYR_BASE}/Makefile.inc
