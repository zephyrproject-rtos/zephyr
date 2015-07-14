PC8253_DEFCONFIGS =                     \
    micro_generic_pc_defconfig          \
    micro_generic_pc_pentium4_defconfig \
    micro_generic_pc_minuteia_defconfig

KBUILD_DEFCONFIG ?= micro_generic_pc_atom_n28xx_defconfig
MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
CONF_FILE = prj_console_float_$(if $(filter $(KBUILD_DEFCONFIG),$(PC8253_DEFCONFIGS)),pc8253,generic).conf

include ${ZEPHYR_BASE}/Makefile.inc
