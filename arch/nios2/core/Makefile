ccflags-y += -I$(srctree)/kernel/include
ccflags-y += -I$(srctree)/arch/$(ARCH)/include

obj-y += reset.o irq_manage.o fatal.o swap.o thread.o \
	 cpu_idle.o irq_offload.o prep_c.o crt0.o \
	 exception.o cache.o

obj-$(CONFIG_IRQ_OFFLOAD) += irq_offload.o
