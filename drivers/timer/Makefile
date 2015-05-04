ifeq (${CONFIG_BSP_GENERIC_PC},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/x86/generic_pc
endif

ifeq (${CONFIG_BSP_QUARK},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/x86/quark
endif

obj-$(CONFIG_PIT) = i8253.o
