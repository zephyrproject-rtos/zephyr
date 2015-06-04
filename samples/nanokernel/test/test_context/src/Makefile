ccflags-y += ${PROJECTINCLUDE} -I${srctree}/samples/include

ifeq (${CONFIG_BSP_GENERIC_PC},y)
ccflags-y +=-I$(srctree)/arch/x86/generic_pc
endif

ifeq (${CONFIG_BSP_QUARK},y)
ccflags-y +=-I$(srctree)/arch/x86/quark
endif

ifeq (${CONFIG_BSP_FSL_FRDM_K64F},y)
ccflags-y +=-I$(srctree)/arch/arm/fsl_frdm_k64f
endif

ifeq (${CONFIG_BSP_TI_LM3S6965},y)
ccflags-y +=-I$(srctree)/arch/arm/ti_lm3s6965
endif

obj-y = context.o
