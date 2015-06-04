ccflags-y +=-I$(srctree)/include/drivers

ifeq (${CONFIG_BSP_FSL_FRDM_K64F},y)
ccflags-y +=-I$(srctree)/arch/arm/fsl_frdm_k64f
endif

ifeq (${CONFIG_BSP_TI_LM3S6965},y)
ccflags-y +=-I$(srctree)/arch/arm/ti_lm3s6965
endif

asflags-y = $(ccflags-y)

obj-y = vector_table.o reset.o \
	prep_c.o scs.o scb.o nmi.o
