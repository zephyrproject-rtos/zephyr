ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/$(ARCH)/$(strip $(CONFIG_PLATFORM))

obj-y = cortex_m_timer.o
obj-$(CONFIG_GDB_INFO) += cortex_m_timer_gdb.o
