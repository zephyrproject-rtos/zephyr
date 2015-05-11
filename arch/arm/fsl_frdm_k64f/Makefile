EXTRA_CFLAGS +=-I$(srctree)/include/bsp
EXTRA_CFLAGS +=-I$(srctree)/include/drivers
EXTRA_CFLAGS +=-I$(srctree)/drivers
EXTRA_CFLAGS +=-I$(srctree)/arch/arm/fsl_frdm_k64f

EXTRA_AFLAGS := ${EXTRA_CFLAGS}


obj-y += system.o
obj-y += nmi_on_reset.o
obj-y += wdog.o

obj-$(CONFIG_IRQ_VECTOR_TABLE_BSP) += irq_vector_table.o
obj-$(CONFIG_SW_ISR_TABLE_BSP) += sw_isr_table.o
