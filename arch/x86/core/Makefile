ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y += -I$(srctree)/kernel/microkernel/include

ifeq ($(COMPILER),clang)
# We rely on GAS for assembling, so don't use the integrated assembler
KBUILD_AFLAGS += -no-integrated-as
endif

# To create dynamic stubs we need to do a .rept on the total number of
# stubs divided by a constant, this prevents GAS from thinking the '/'
# character starts a comment
KBUILD_AFLAGS += -Wa,--divide

ifndef CONFIG_X86_IAMCU
obj-y += i386_sysV_abi/
else
obj-y += iamcu_abi/
endif

obj-y += fatal.o cpuhalt.o \
	msr.o dynamic.o intconnect.o \
	excconnect.o sys_fatal_error_handler.o \
	crt0.o atomic.o cache_s.o cache.o excstub.o

obj-$(CONFIG_IRQ_OFFLOAD) += irq_offload.o
obj-$(CONFIG_FP_SHARING) += float.o
obj-$(CONFIG_MICROKERNEL) += strtask.o
obj-$(CONFIG_ERRNO) += errno.o
obj-$(CONFIG_GDT_DYNAMIC) += gdt.o
