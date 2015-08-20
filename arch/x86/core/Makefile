ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y += -I$(srctree)/kernel/microkernel/include

obj-y = gdt.o thread.o fatal.o cpuhalt.o \
	excstub.o swap.o intboiexit.o msr.o \
	excconnect.o inthndlset.o sys_fatal_error_handler.o \
	crt0.o driver_static_irq_stubs.o

ifeq ($(CONFIG_LOCK_INSTRUCTION_UNSUPPORTED),y)
obj-y += atomic_nolock.o
else
obj-y += atomic.o
endif

ifneq ($(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED),y)
obj-y += cache_s.o
else
obj-y += cache.o
endif

obj-$(CONFIG_FP_SHARING) += float.o
obj-$(CONFIG_UNALIGNED_WRITE_UNSUPPORTED) += unaligned.o
obj-$(CONFIG_MICROKERNEL) += strtask.o

ifneq ($(CONFIG_NO_ISRS),y)
obj-y += intconnect.o intstub.o
endif
