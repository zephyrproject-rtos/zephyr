EXTRA_CFLAGS +=-I$(srctree)/include/bsp
EXTRA_CFLAGS +=-I$(srctree)/include/drivers
EXTRA_CFLAGS +=-I$(srctree)/drivers
EXTRA_CFLAGS +=-I$(srctree)/arch/arm/ti_lm3s6965

EXTRA_AFLAGS := ${EXTRA_CFLAGS}


obj-y += system.o
obj-y += nmi_on_reset.o
obj-y += scp.o

obj-$(CONFIG_IRQ_VECTOR_TABLE_BSP) += irq_vector_table.o
obj-$(CONFIG_SW_ISR_TABLE_BSP) += sw_isr_table.o
