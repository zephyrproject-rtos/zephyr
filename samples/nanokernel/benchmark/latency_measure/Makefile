TIMO_BASE = $(strip $(subst /samples/nanokernel/benchmark/latency_measure,,$(shell pwd) ))

PC8253_BSP_VARIANTS=generic_pc pentium4 minuteia

BSP ?= generic_pc
KERNEL_TYPE = nano
CONF_OVERLAY = prj.conf
SOURCE_DIR = $(TIMO_BASE)/samples/microkernel/benchmark/latency_measure

include $(TIMO_BASE)/Makefile.inc
