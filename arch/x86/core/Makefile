ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y += -I$(srctree)/kernel/microkernel/include

# To create dynamic stubs we need to do a .rept on the total number of
# stubs divided by a constant, this prevents GAS from thinking the '/'
# character starts a comment
ifeq ($(COMPILER),clang)
KBUILD_AFLAGS += -no-integrated-as -Wa,--divide
else
KBUILD_AFLAGS += -Wa,--divide
endif

obj-y = i386_sysV_abi/

obj-y += gdt.o fatal.o cpuhalt.o \
	msr.o dynamic.o \
	excconnect.o sys_fatal_error_handler.o \
	crt0.o driver_static_irq_stubs.o \
	atomic.o cache_s.o cache.o

obj-$(CONFIG_FP_SHARING) += float.o
obj-$(CONFIG_MICROKERNEL) += strtask.o

