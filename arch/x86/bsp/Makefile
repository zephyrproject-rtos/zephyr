ifeq (${CONFIG_BSP_GENERIC_PC},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/x86/generic_pc
EXTRA_AFLAGS +=-I$(srctree)/arch/x86/generic_pc
endif

ifeq (${CONFIG_BSP_QUARK},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/x86/quark
endif

obj-y = crt0.o sysFatalErrorHandler.o driver_static_irq_stubs.o

ifneq ($(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED),y)
obj-y += cache_s.o
endif

obj-$(CONFIG_LOAPIC) += systemApic.o
obj-$(CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED) += cache.o
obj-$(CONFIG_PIC) += systemPic.o
obj-$(CONFIG_PIC) += i8259Boi.o
obj-$(CONFIG_TEST_RANDOM_GENERATOR) += rand32.o
