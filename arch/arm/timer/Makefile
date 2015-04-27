EXTRA_CFLAGS +=-I$(srctree)/include/bsp
EXTRA_CFLAGS +=-I$(srctree)/kernel/common/bsp
EXTRA_CFLAGS +=-I$(srctree)/drivers
EXTRA_CFLAGS +=-I$(srctree)/arch/arm/fsl_frdm_k64f

obj-y = systick.o
obj-$(CONFIG_GDB_INFO) += systick_gdb.o
