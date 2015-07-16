ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/drivers
ccflags-y +=-I$(srctree)/arch/arm/platforms/fsl_frdm_k64f

asflags-y := ${ccflags-y}


obj-y += system.o
obj-y += nmi_on_reset.o
obj-y += wdog.o

obj-$(CONFIG_IRQ_VECTOR_TABLE_BSP) += irq_vector_table.o
