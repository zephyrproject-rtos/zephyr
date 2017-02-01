ccflags-y +=-I$(srctree)/include/debug
CFLAGS_gdb_server.o =-I$(srctree)/include/drivers

obj-y =
obj-$(CONFIG_MEM_SAFE_CHECK_BOUNDARIES) += mem_safe_check_boundaries.o
obj-$(CONFIG_GDB_SERVER) += gdb_server.o

ifeq ($(CONFIG_OPENOCD_SUPPORT),y)
lib-y += openocd.o
ldflags-y += --undefined=_kernel_openocd_size_t_size
ldflags-y += --undefined=_kernel_openocd_offsets
endif
