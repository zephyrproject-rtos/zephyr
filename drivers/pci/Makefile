EXTRA_CFLAGS += -I$(srctree)/include/drivers

EXTRA_CFLAGS +=-I$(srctree)/arch/$(ARCH)
EXTRA_CFLAGS +=-I$(srctree)/arch/$(ARCH)/$(strip $(CONFIG_BSP_DIR))

obj-y = pci.o  pci_config.o  pci_interface.o
