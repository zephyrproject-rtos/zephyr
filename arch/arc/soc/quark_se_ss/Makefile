ccflags-y +=-I$(srctree)/arch/x86/soc/
ccflags-y +=-I$(srctree)/include
ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/drivers
ccflags-$(CONFIG_ADC) +=-I$(srctree)/drivers/adc

asflags-y := ${ccflags-y}

obj-y = soc.o soc_config.o
obj-$(CONFIG_IRQ_VECTOR_TABLE_BSP) += irq_vector_table.o
obj-$(CONFIG_SW_ISR_TABLE_BSP) += sw_isr_table.o
