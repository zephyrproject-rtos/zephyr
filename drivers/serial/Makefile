ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/$(ARCH)
ccflags-y +=-I$(srctree)/arch/$(ARCH)/$(strip $(CONFIG_PLATFORM))


obj-$(CONFIG_NS16550) = ns16550.o
obj-$(CONFIG_K20_UART)     = k20UartDrv.o
obj-$(CONFIG_STELLARIS_UART) = stellarisUartDrv.o
obj-$(CONFIG_NSIM) = nsim_uart.o
