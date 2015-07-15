PC8253_DEFCONFIGS = basic_minuteia

MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= basic_atom
CONF_FILE = prj_$(if $(filter $(PLATFORM_CONFIG),$(PC8253_DEFCONFIGS)),pc8253,generic).conf

include ${ZEPHYR_BASE}/Makefile.inc
