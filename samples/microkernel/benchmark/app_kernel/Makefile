PC8253_DEFCONFIGS = generic_pc_minuteia

PLATFORM_CONFIG ?= generic_pc_atom_n28xx
MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
CONF_FILE = prj_console_float_$(if $(filter $(PLATFORM_CONFIG),$(PC8253_DEFCONFIGS)),pc8253,generic).conf

include ${ZEPHYR_BASE}/Makefile.inc
