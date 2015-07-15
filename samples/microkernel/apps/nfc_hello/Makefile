KERNEL_TYPE ?= nano
PLATFORM_CONFIG ?= basic_atom
CONF_FILE = prj.conf
SOURCE_DIR = $(ZEPHYR_BASE)/samples/microkernel/apps/nfc_hello/src/

QEMU_EXTRA_FLAGS += -serial tcp:localhost:8888

include ${ZEPHYR_BASE}/Makefile.inc
