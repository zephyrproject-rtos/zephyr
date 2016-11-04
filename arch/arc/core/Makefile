ccflags-y += -I$(srctree)/kernel/unified/include
ccflags-y +=-I$(srctree)/arch/$(ARCH)/include

obj-y += thread.o thread_entry_wrapper.o \
	cpu_idle.o  fast_irq.o  fatal.o  fault.o  \
	fault_s.o  irq_manage.o cache.o  \
	isr_wrapper.o  regular_irq.o  swap_macros.h  swap.o \
	sys_fatal_error_handler.o

obj-y += prep_c.o \
         reset.o \
         vector_table.o

obj-$(CONFIG_IRQ_OFFLOAD) += irq_offload.o

# Some ARC cores like the EM4 lack the atomic LLOCK/SCOND and
# can't use these.
obj-$(CONFIG_ATOMIC_OPERATIONS_CUSTOM) += atomic.o
obj-$(CONFIG_IRQ_VECTOR_TABLE_BSP) += irq_vector_table.o
obj-$(CONFIG_SW_ISR_TABLE) += sw_isr_table.o
