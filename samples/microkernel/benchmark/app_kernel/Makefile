TIMO_BASE = $(strip $(subst /samples/microkernel/benchmark/app_kernel,,$(shell pwd) ))

PC8253_BSP_VARIANTS=generic_pc pentium4 minuteia

BSP ?= generic_pc
VPFILE = prj.vpf
KERNEL_TYPE = micro
CONF_OVERLAY = prj_console_float_$(if $(filter $(BSP),$(PC8253_BSP_VARIANTS)),pc8253,generic).conf

include ${TIMO_BASE}/Makefile.inc
