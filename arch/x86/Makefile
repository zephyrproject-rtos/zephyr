
obj-y = crt0.o driver_static_irq_stubs.o

ifneq ($(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED),y)
obj-y += cache_s.o
endif

obj-$(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED) += cache.o

obj-y += core/
obj-$(CONFIG_MICROKERNEL) += task/
obj-$(CONFIG_PLATFORM_QUARK) += platforms/ia32_pci/
obj-$(CONFIG_PLATFORM_GENERIC_PC) += platforms/ia32/
