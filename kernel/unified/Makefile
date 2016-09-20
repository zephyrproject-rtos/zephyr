ccflags-y += -I$(srctree)/kernel/unified/include

asflags-y := ${ccflags-y}

obj-y =
obj-y += $(strip \
	sys_clock.o \
	thread.o \
	init.o \
	sem.o \
	version.o \
	device.o \
	thread_abort.o \
)

obj-y += $(strip \
	sched.o \
	mutex.o \
)
obj-y += $(strip \
	lifo.o \
	fifo.o \
	stack.o \
	mem_map.o \
	mem_pool.o \
	msg_q.o \
	mailbox.o \
	mem_pool.o \
	event.o \
	pipes.o \
	offload.o \
)

obj-$(CONFIG_INT_LATENCY_BENCHMARK) += int_latency_bench.o
obj-$(CONFIG_STACK_CANARIES) += compiler_stack_protect.o
obj-$(CONFIG_SYS_POWER_MANAGEMENT) += idle.o
obj-$(CONFIG_NANO_TIMERS) += timer.o
obj-$(CONFIG_KERNEL_EVENT_LOGGER) += event_logger.o
obj-$(CONFIG_KERNEL_EVENT_LOGGER) += kernel_event_logger.o
obj-$(CONFIG_RING_BUFFER) += ring_buffer.o
obj-$(CONFIG_ATOMIC_OPERATIONS_C) += atomic_c.o
obj-$(CONFIG_ERRNO) += errno.o
obj-$(CONFIG_NANO_WORKQUEUE) += work_q.o

obj-y += legacy/
