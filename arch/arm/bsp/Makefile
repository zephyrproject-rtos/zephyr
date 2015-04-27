EXTRA_CFLAGS +=-I$(srctree)/include/bsp
EXTRA_CFLAGS +=-I$(srctree)/kernel/common/bsp
EXTRA_CFLAGS +=-I$(srctree)/drivers

ifeq (${CONFIG_BSP_FSL_FRDM_K64F},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/arm/fsl_frdm_k64f
endif

ifeq (${CONFIG_BSP_TI_LM3S6965},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/arm/ti_lm3s6965
endif

obj-y = CortexM/
obj-y += sysFatalErrorHandler.o rand32.o
