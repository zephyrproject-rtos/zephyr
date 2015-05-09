TIMO_BASE = $(strip $(subst /samples/microkernel/apps/hello_world,,$(shell pwd) ))
BSP ?= generic_pc
VPFILE = prj.vpf
KERNEL_TYPE = micro
CONF_OVERLAY = prj_$(ARCH).conf

include ${TIMO_BASE}/Makefile.inc
