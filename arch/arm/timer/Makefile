ccflags-y +=-I$(srctree)/include/bsp
ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/$(ARCH)/$(strip $(CONFIG_BSP_DIR))

obj-y = systick.o
obj-$(CONFIG_GDB_INFO) += systick_gdb.o
