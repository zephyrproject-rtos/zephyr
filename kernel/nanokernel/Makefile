EXTRA_CFLAGS +=-I$(srctree)/kernel/nanokernel/include
EXTRA_CFLAGS +=-I$(srctree)/kernel/microkernel/include


EXTRA_AFLAGS := ${EXTRA_CFLAGS}

obj-y = nanofiber.o nano_lifo.o nano_context_data.o\
	nano_mwfifo.o nano_stack.o nano_timer.o \
	nanocontextentry.o nanoinit.o nano_sema.o \
	version.o init.o ctors.o

obj-$(CONFIG_ENHANCED_SECURITY) += string_s.o
obj-$(CONFIG_STACK_CANARIES) += compiler_stack_protect.o
obj-$(CONFIG_INT_LATENCY_BENCHMARK) += int_latency_bench.o
obj-$(CONFIG_PERFORMANCE_METRICS) += nanometrics.o
obj-$(CONFIG_ADVANCED_POWER_MANAGEMENT) += idle.o
obj-$(CONFIG_NANOKERNEL) += nanotime.o

