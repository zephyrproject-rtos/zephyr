ccflags-y += -I$(srctree)/kernel/unified/include

asflags-y := ${ccflags-y}

obj-y =
obj-y += $(strip \
	version.o \
)

lib-y =
lib-y += $(strip \
	sys_clock.o \
	thread.o \
	init.o \
	sem.o \
	device.o \
	thread_abort.o \
)

lib-y += $(strip \
	sched.o \
	mutex.o \
)
lib-y += $(strip \
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

lib-$(CONFIG_INT_LATENCY_BENCHMARK) += int_latency_bench.o
lib-$(CONFIG_STACK_CANARIES) += compiler_stack_protect.o
lib-$(CONFIG_SYS_POWER_MANAGEMENT) += idle.o
lib-$(CONFIG_NANO_TIMERS) += timer.o
lib-$(CONFIG_KERNEL_EVENT_LOGGER) += event_logger.o
lib-$(CONFIG_KERNEL_EVENT_LOGGER) += kernel_event_logger.o
lib-$(CONFIG_RING_BUFFER) += ring_buffer.o
lib-$(CONFIG_ATOMIC_OPERATIONS_C) += atomic_c.o
lib-$(CONFIG_ERRNO) += errno.o
lib-$(CONFIG_NANO_WORKQUEUE) += work_q.o

obj-y += legacy/
