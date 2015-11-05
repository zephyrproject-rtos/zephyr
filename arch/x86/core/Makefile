ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y += -I$(srctree)/kernel/microkernel/include

# To create dynamic stubs we need to do a .rept on the total number of
# stubs divided by a constant, this prevents GAS from thinking the '/'
# character starts a comment
KBUILD_AFLAGS += -Wa,--divide

obj-y = gdt.o thread.o fatal.o cpuhalt.o \
	excstub.o swap.o msr.o dynamic.o \
	excconnect.o sys_fatal_error_handler.o \
	crt0.o driver_static_irq_stubs.o

obj-y += atomic_nolock.o atomic.o
obj-y += cache_s.o cache.o
obj-y += intconnect.o intstub.o

obj-$(CONFIG_FP_SHARING) += float.o
obj-$(CONFIG_MICROKERNEL) += strtask.o

