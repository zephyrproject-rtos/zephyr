ccflags-y +=-I$(srctree)/include/debug

obj- = dummy.o

ifeq ($(CONFIG_OPENOCD_SUPPORT),y)
lib-y += openocd.o
ldflags-y += --undefined=_kernel_openocd_size_t_size
ldflags-y += --undefined=_kernel_openocd_offsets
endif
