ccflags-y +=-I$(srctree)/arch/arc/platforms/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))
ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/drivers
asflags-y +=-I$(srctree)/arch/arc/platforms/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))

asflags-y := ${ccflags-y}

obj-y = system.o
obj-$(CONFIG_IRQ_VECTOR_TABLE_BSP) += irq_vector_table.o
obj-$(CONFIG_SW_ISR_TABLE_BSP) += sw_isr_table.o
