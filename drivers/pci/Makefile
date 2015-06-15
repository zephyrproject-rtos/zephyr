ccflags-y += -I$(srctree)/include/drivers

ccflags-y +=-I$(srctree)/arch/$(ARCH)
ccflags-y +=-I$(srctree)/arch/$(ARCH)/$(strip $(CONFIG_PLATFORM))

obj-$(CONFIG_PCI) = pci.o pci_config.o pci_interface.o
