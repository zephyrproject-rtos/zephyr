ccflags-y +=-I$(srctree)/include/bsp
ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/drivers
ccflags-y +=-I$(srctree)/arch/arm/ti_lm3s6965

asflags-y := ${ccflags-y}


obj-y += system.o
obj-y += nmi_on_reset.o
obj-y += scp.o

obj-$(CONFIG_IRQ_VECTOR_TABLE_BSP) += irq_vector_table.o
