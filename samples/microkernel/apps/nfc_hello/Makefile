BSP ?= generic_pc
KERNEL_TYPE ?= nano
CONF_FILE = prj.conf
SOURCE_DIR = $(ZEPHYR_BASE)/samples/nfc_hello/src/

QEMU_EXTRA_FLAGS += -serial tcp:localhost:8888

include ${ZEPHYR_BASE}/Makefile.inc
