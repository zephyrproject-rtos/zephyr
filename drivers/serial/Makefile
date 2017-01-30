ccflags-$(CONFIG_UART_QMSI) +=-I$(CONFIG_QMSI_INSTALL_PATH)/include
ccflags-y +=-I$(srctree)/drivers

obj-$(CONFIG_UART_NS16550)	+= uart_ns16550.o
obj-$(CONFIG_UART_MCUX)		+= uart_mcux.o
obj-$(CONFIG_UART_MCUX_LPUART)	+= uart_mcux_lpuart.o
obj-$(CONFIG_UART_STELLARIS)	+= uart_stellaris.o
obj-$(CONFIG_UART_NSIM)		+= uart_nsim.o
obj-$(CONFIG_UART_ATMEL_SAM3)	+= uart_atmel_sam3.o
obj-$(CONFIG_UART_QMSI)		+= uart_qmsi.o
obj-$(CONFIG_UART_STM32)	+= uart_stm32.o
obj-$(CONFIG_UART_NRF5)         += uart_nrf5.o
obj-$(CONFIG_UART_ALTERA_JTAG)  += uart_altera_jtag.o
obj-$(CONFIG_UART_CC32XX)	+= uart_cc32xx.o
obj-$(CONFIG_UART_CMSDK_APB)	+= uart_cmsdk_apb.o
obj-$(CONFIG_UART_RISCV_QEMU)	+= uart_riscv_qemu.o
