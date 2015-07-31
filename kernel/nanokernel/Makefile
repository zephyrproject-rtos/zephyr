ccflags-y +=-I$(srctree)/kernel/nanokernel/include
ccflags-y +=-I$(srctree)/kernel/microkernel/include


asflags-y := ${ccflags-y}

obj-y = nano_fiber.o nano_lifo.o \
	nano_fifo.o nano_stack.o nano_sys_clock.o \
	nano_context.o nano_init.o nano_sema.o \
	version.o  ctors.o device.o

obj-$(CONFIG_STACK_CANARIES) += compiler_stack_protect.o
obj-$(CONFIG_INT_LATENCY_BENCHMARK) += int_latency_bench.o
obj-$(CONFIG_ADVANCED_POWER_MANAGEMENT) += idle.o
obj-$(CONFIG_NANO_TIMERS) += nano_timer.o
obj-$(CONFIG_EVENT_LOGGER) += event_logger.o
obj-$(CONFIG_KERNEL_PROFILER) += profiler.o
