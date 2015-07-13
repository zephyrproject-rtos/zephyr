KBUILD_DEFCONFIG ?= micro_generic_pc_defconfig
MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
CONF_FILE = prj_$(ARCH).conf
QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr

include $(ZEPHYR_BASE)/Makefile.inc
