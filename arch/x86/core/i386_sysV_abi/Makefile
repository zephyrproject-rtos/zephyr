ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y += -I$(srctree)/kernel/microkernel/include

# see explanation for flags in arch/x86/core/Makefile
ifeq ($(COMPILER),clang)
KBUILD_AFLAGS += -no-integrated-as
endif
KBUILD_AFLAGS += -Wa,--divide

obj-y = excstub.o intconnect.o intstub.o swap.o thread.o
