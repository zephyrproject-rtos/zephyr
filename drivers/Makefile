subdir-ccflags-y += -I$(srctree)/include/drivers
subdir-ccflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))
subdir-asflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))

obj-y = console/
obj-y += random/
obj-y += serial/
obj-y += timer/
obj-y += interrupt_controller/
obj-$(CONFIG_PCI) += pci/
obj-$(CONFIG_BLUETOOTH) += bluetooth/
obj-$(CONFIG_UART_SIMPLE) += simple/
obj-$(CONFIG_SPI) += spi/
obj-$(CONFIG_GPIO) += gpio/
