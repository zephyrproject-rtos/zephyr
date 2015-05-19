PC8253_BSP_VARIANTS=generic_pc pentium4 minuteia

BSP ?= generic_pc
VPFILE = $(TIMO_BASE)/samples/microkernel/benchmark/sys_kernel/prj.vpf
KERNEL_TYPE = micro
CONF_FILE = prj_console_$(if $(filter $(BSP),$(PC8253_BSP_VARIANTS)),pc8253,generic).conf
SOURCE_DIR = $(TIMO_BASE)/samples/nanokernel/benchmark/sys_kernel/src/

include ${TIMO_BASE}/Makefile.inc
