ccflags-y +=-I$(srctree)/include/misc/debug
CFLAGS_gdb_server.o =-I$(srctree)/include/drivers

obj-y =
obj-$(CONFIG_MEM_SAFE_CHECK_BOUNDARIES) += mem_safe_check_boundaries.o
obj-$(CONFIG_GDB_SERVER) += gdb_server.o
