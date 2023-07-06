# Copyright (c) 2016 Wind River Systems, Inc.
# SPDX-License-Identifier: Apache-2.0

menu "USB CDC ACM Class support"

config USB_CDC_ACM
	bool "USB CDC ACM Class support"
	default y
	select USB_COMPOSITE_DEVICE
	depends on SERIAL
	depends on DT_HAS_ZEPHYR_CDC_ACM_UART_ENABLED
	select SERIAL_HAS_DRIVER
	select SERIAL_SUPPORT_INTERRUPT
	select RING_BUFFER
	select UART_INTERRUPT_DRIVEN
	help
	  USB CDC ACM class support.

if USB_CDC_ACM

config USB_CDC_ACM_RINGBUF_SIZE
	int "USB CDC ACM ring buffer size"
	default 1024
	help
	  USB CDC ACM ring buffer size

config CDC_ACM_INTERRUPT_EP_MPS
	int
	default 16
	help
	  CDC ACM class interrupt IN endpoint size

config CDC_ACM_BULK_EP_MPS
	int
	default 512 if USB_DC_HAS_HS_SUPPORT
	default 64
	help
	  CDC ACM class bulk endpoints size

config CDC_ACM_IAD
	bool "Force using Interface Association Descriptor"
	default y
	help
	  IAD should not be required for non-composite CDC ACM device,
	  but Windows 7 fails to properly enumerate without it.
	  Enable if you want CDC ACM to work with Windows 7.

config CDC_ACM_DTE_RATE_CALLBACK_SUPPORT
	bool "Support callbacks when the USB host changes the virtual baud rate"
	default BOOTLOADER_BOSSA
	help
	  If set, enables support for a callback that is invoked when the
	  remote host changes the virtual baud rate. This is used
	  by Arduino style programmers to reset the device into the
	  bootloader.

module = USB_CDC_ACM
module-str = usb cdc acm
source "subsys/logging/Kconfig.template.log_config"

endif # USB_CDC_ACM

endmenu
