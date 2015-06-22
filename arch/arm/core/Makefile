ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y += -I$(srctree)/kernel/microkernel/include

asflags-y := ${ccflags-y}

obj-y = atomic.o exc_exit.o ffs.o irq_init.o \
	fiber_abort.o swap.o basepri.o \
	fault.o gdb_stub_irq_vector_table.o \
	irq_manage.o context.o cpu_idle.o \
	fault_s.o gdb_stub.o isr_wrapper.o \
	fatal.o

obj-$(CONFIG_MICROKERNEL) += task_abort.o
