EXTRA_CFLAGS +=-I$(srctree)/include/drivers
EXTRA_CFLAGS +=-I$(srctree)/arch/$(ARCH)
EXTRA_CFLAGS +=-I$(srctree)/arch/$(ARCH)/$(strip $(CONFIG_BSP_DIR))


obj-$(CONFIG_DRV_NS16550) = ns16550.o
obj-$(CONFIG_DRV_K20_UART)     = k20UartDrv.o
obj-$(CONFIG_DRV_STELLARIS_UART) = stellarisUartDrv.o
