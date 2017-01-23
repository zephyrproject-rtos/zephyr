ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/arm/soc/$(SOC_PATH)
ccflags-y +=-I$(srctree)/kernel/include
ccflags-y +=-I$(srctree)/include/

asflags-y = $(ccflags-y)

obj-y = vector_table.o reset.o \
	nmi_on_reset.o prep_c.o scs.o scb.o nmi.o \
	exc_manage.o

obj-$(CONFIG_IRQ_VECTOR_TABLE_SOC) += irq_vector_table.o
obj-$(CONFIG_SW_ISR_TABLE) += sw_isr_table.o
