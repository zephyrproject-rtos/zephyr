ccflags-y += -I$(srctree)/kernel/include
asflags-y := ${ccflags-y}

obj-y = exc_exit.o irq_init.o swap.o fault.o irq_manage.o thread.o \
	cpu_idle.o fault_s.o fatal.o sys_fatal_error_handler.o thread_abort.o

obj-$(CONFIG_GEN_SW_ISR_TABLE) += isr_wrapper.o
obj-$(CONFIG_CPLUSPLUS) += __aeabi_atexit.o
obj-$(CONFIG_IRQ_OFFLOAD) += irq_offload.o
obj-$(CONFIG_CPU_CORTEX_M) += cortex_m/
