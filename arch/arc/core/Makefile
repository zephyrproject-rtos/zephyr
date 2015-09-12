ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y +=-I$(srctree)/arch/$(ARCH)/include
ccflags-y += -I$(srctree)/kernel/microkernel/include

obj-y += atomic.o thread.o thread_entry_wrapper.o \
	cpu_idle.o  fast_irq.o  fatal.o  fault.o  \
	fault_s.o  irq_manage.o  \
	isr_wrapper.o  regular_irq.o  swap_macros.h  swap.o \
	sys_fatal_error_handler.o

obj-y += prep_c.o \
         reset.o \
         vector_table.o
