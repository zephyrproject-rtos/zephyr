ccflags-y += -I$(srctree)/kernel/nanokernel/include
ccflags-y +=-I$(srctree)/arch/$(ARCH)/include
ccflags-y += -I$(srctree)/kernel/microkernel/include

obj-y += reset.o irq_manage.o fatal.o swap.o thread.o \
	 cpu_idle.o irq_offload.o prep_c.o crt0.o

obj-$(CONFIG_IRQ_OFFLOAD) += irq_offload.o

