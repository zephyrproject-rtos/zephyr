ccflags-y +=-I$(srctree)/arch/x86/$(strip $(CONFIG_BSP_DIR))
asflags-y +=-I$(srctree)/arch/x86/$(strip $(CONFIG_BSP_DIR))

obj-y = crt0.o sysFatalErrorHandler.o driver_static_irq_stubs.o

ifneq ($(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED),y)
obj-y += cache_s.o
endif

obj-$(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED) += cache.o
obj-$(CONFIG_X86_TSC_RANDOM_GENERATOR) += rand32-timestamp.o
