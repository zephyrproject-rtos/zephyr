PC8253_BSP_VARIANTS=generic_pc pentium4 minuteia

BSP ?= generic_pc
MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
CONF_FILE = prj_console_$(if $(filter $(BSP),$(PC8253_BSP_VARIANTS)),pc8253,generic).conf
SOURCE_DIR = $(ZEPHYR_BASE)/samples/nanokernel/benchmark/sys_kernel/src/

include ${ZEPHYR_BASE}/Makefile.inc
