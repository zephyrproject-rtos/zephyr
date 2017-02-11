ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/arm/soc/$(SOC_PATH)
ccflags-y +=-I$(srctree)/kernel/include
ccflags-y +=-I$(srctree)/include/

asflags-y = $(ccflags-y)

obj-y = vector_table.o reset.o \
	nmi_on_reset.o prep_c.o scb.o nmi.o \
	exc_manage.o

