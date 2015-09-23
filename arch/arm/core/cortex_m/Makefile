ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))

asflags-y = $(ccflags-y)

obj-y = vector_table.o reset.o \
	prep_c.o scs.o scb.o nmi.o \
	sw_isr_table.o
