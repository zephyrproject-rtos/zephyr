ifeq (${CONFIG_BSP_GENERIC_PC},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/x86
EXTRA_CFLAGS +=-I$(srctree)/arch/x86/generic_pc
endif

ifeq (${CONFIG_BSP_QUARK},y)
EXTRA_CFLAGS +=-I$(srctree)/arch/x86
EXTRA_CFLAGS +=-I$(srctree)/arch/x86/quark
endif
EXTRA_CFLAGS += -I$(srctree)/include/drivers

obj-y = pci.o  pci_config.o  pci_interface.o
