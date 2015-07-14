PC8253_DEFCONFIGS = micro_generic_pc_minuteia_defconfig

MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
KBUILD_DEFCONFIG ?= $(KERNEL_TYPE)_generic_pc_atom_n28xx_defconfig
CONF_FILE = prj_console_$(if $(filter $(KBUILD_DEFCONFIG),$(PC8253_DEFCONFIGS)),pc8253,generic).conf
SOURCE_DIR = $(ZEPHYR_BASE)/samples/nanokernel/benchmark/sys_kernel/src/

include ${ZEPHYR_BASE}/Makefile.inc
