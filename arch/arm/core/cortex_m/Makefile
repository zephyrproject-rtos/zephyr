ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(PLATFORM_NAME)

asflags-y = $(ccflags-y)

obj-y = vector_table.o reset.o \
	prep_c.o scs.o scb.o nmi.o \
	sw_isr_table.o

obj-$(CONFIG_ERRNO) += errno.o
