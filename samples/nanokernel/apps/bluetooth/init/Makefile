KERNEL_TYPE = nano
BOARD ?= qemu_x86
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR = $(ZEPHYR_BASE)/samples/bluetooth/init/src/
QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr

include $(ZEPHYR_BASE)/Makefile.inc
