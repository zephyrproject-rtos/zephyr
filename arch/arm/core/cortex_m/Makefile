ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/arm/soc/$(SOC_PATH)
ifeq ($(CONFIG_KERNEL_V2),y)
ccflags-y +=-I$(srctree)/kernel/unified/include
else
ccflags-y +=-I$(srctree)/kernel/nanokernel/include
endif

asflags-y = $(ccflags-y)

obj-y = vector_table.o reset.o \
	nmi_on_reset.o prep_c.o scs.o scb.o nmi.o \
	exc_manage.o

obj-$(CONFIG_IRQ_VECTOR_TABLE_SOC) += irq_vector_table.o
obj-$(CONFIG_SW_ISR_TABLE) += sw_isr_table.o
