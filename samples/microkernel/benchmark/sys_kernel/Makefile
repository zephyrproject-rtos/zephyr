PC8253_DEFCONFIGS = generic_pc_minuteia

MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= generic_pc_atom_n28xx
CONF_FILE = prj_console_$(if $(filter $(PLATFORM_CONFIG),$(PC8253_DEFCONFIGS)),pc8253,generic).conf
SOURCE_DIR = $(ZEPHYR_BASE)/samples/nanokernel/benchmark/sys_kernel/src/

include ${ZEPHYR_BASE}/Makefile.inc
