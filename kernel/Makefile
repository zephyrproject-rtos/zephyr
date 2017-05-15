ccflags-y += -I$(srctree)/kernel/include

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
	idle.o \
	sched.o \
	mutex.o \
	queue.o \
	stack.o \
	mem_slab.o \
	mempool.o \
	msg_q.o \
	mailbox.o \
	alert.o \
	pipes.o \
	errno.o \
	work_q.o \
	system_work_q.o \
)

lib-$(CONFIG_INT_LATENCY_BENCHMARK) += int_latency_bench.o
lib-$(CONFIG_STACK_CANARIES) += compiler_stack_protect.o
lib-$(CONFIG_SYS_CLOCK_EXISTS) += timer.o
lib-$(CONFIG_ATOMIC_OPERATIONS_C) += atomic_c.o
lib-$(CONFIG_POLL) += poll.o
