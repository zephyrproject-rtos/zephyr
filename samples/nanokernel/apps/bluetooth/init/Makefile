BSP ?= generic_pc
KERNEL_TYPE = nano
CONF_OVERLAY = prj_$(ARCH).conf
SOURCE_DIR = $(TIMO_BASE)/samples/bluetooth/init/src/
QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr

include $(TIMO_BASE)/Makefile.inc
