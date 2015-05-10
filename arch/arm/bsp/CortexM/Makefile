EXTRA_CFLAGS +=-I$(srctree)/include/drivers

ifeq (${CONFIG_BSP_FSL_FRDM_K64F},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/arm/fsl_frdm_k64f
endif

ifeq (${CONFIG_BSP_TI_LM3S6965},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/arm/ti_lm3s6965
endif

EXTRA_AFLAGS = $(EXTRA_CFLAGS)

obj-y = vector_table.o reset.o \
	prep_c.o scs.o scb.o nmi.o
