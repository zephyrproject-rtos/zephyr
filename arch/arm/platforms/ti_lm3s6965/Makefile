ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/drivers
ccflags-y +=-I$(srctree)/arch/arm/platforms/ti_lm3s6965

asflags-y := ${ccflags-y}


obj-y += platform_config.o
obj-y += system.o
obj-y += nmi_on_reset.o
obj-y += scp.o

obj-$(CONFIG_IRQ_VECTOR_TABLE_PLATFORM) += irq_vector_table.o
