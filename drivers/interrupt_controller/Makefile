EXTRA_CFLAGS +=-I$(srctree)/include/drivers
EXTRA_CFLAGS +=-I$(srctree)/arch/$(ARCH)
EXTRA_CFLAGS +=-I$(srctree)/arch/$(ARCH)/$(strip $(CONFIG_BSP_DIR))

obj-${CONFIG_PIC}${CONFIG_SHUTOFF_PIC} = i8259.o
