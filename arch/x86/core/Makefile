EXTRA_CFLAGS += -I$(srctree)/kernel/nanokernel/include
EXTRA_CFLAGS += -I$(srctree)/kernel/microkernel/include

obj-y = gdt.o nanocontext.o nanofatal.o cpuhalt.o \
	excstub.o swap.o ffs.o intboiexit.o msr.o \
	excconnect.o inthndlset.o

ifeq ($(CONFIG_LOCK_INSTRUCTION_UNSUPPORTED),y)
obj-y += nanoatomic_nolock.o
else
obj-y += nanoatomic.o
endif

obj-$(CONFIG_FP_SHARING) += nanofloat.o
obj-$(CONFIG_UNALIGNED_WRITE_UNSUPPORTED) += unaligned.o

ifneq ($(CONFIG_NO_ISRS),y)
obj-y += intconnect.o intstub.o
endif
