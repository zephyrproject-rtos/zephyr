
obj-y = crt0.o sys_fatal_error_handler.o driver_static_irq_stubs.o

ifneq ($(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED),y)
obj-y += cache_s.o
endif

obj-$(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED) += cache.o

obj-y += core/
obj-$(CONFIG_MICROKERNEL) += task/
obj-$(CONFIG_BSP_QUARK) += quark/
obj-$(CONFIG_BSP_GENERIC_PC) += generic_pc/
