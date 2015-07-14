KERNEL_TYPE = nano
PLATFORM_CONFIG ?= generic_pc_atom_n28xx
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR = $(ZEPHYR_BASE)/samples/bluetooth/init/src/
QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr

include $(ZEPHYR_BASE)/Makefile.inc
