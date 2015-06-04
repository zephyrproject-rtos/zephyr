ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y += -I$(srctree)/kernel/microkernel/include
ccflags-y += -I$(srctree)/target/src/bsp/arm

asflags-y := ${ccflags-y}

obj-y = atomic.o exc_exit.o ffs.o irq_init.o \
	nano_fiber_abort.o swap.o basepri.o \
	fault.o gdb_stub_irq_vector_table.o \
	irq_manage.o nanocontext.o cpu_idle.o \
	fault_s.o gdb_stub.o isr_wrapper.o \
	nanofatal.o

obj-$(CONFIG_MICROKERNEL) += task_abort.o
