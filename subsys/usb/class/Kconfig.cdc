# Copyright (c) 2016 Wind River Systems, Inc.
# SPDX-License-Identifier: Apache-2.0

DT_COMPAT_ZEPHYR_CDC_ACM_UART := zephyr,cdc-acm-uart

menu "USB CDC ACM Class support"

config USB_CDC_ACM_HAS_DT_COMPAT
	bool
	default $(dt_compat_enabled,$(DT_COMPAT_ZEPHYR_CDC_ACM_UART))
	imply USB_CDC_ACM
	help
	  Helper symbol to select USB_CDC_ACM if compatible node is enabled.

config USB_CDC_ACM
	bool "USB CDC ACM Class support"
	depends on SERIAL
	select SERIAL_HAS_DRIVER
	select SERIAL_SUPPORT_INTERRUPT
	select RING_BUFFER
	select UART_INTERRUPT_DRIVEN
	help
	  USB CDC ACM class support.
	  Default UART device name is "CDC_ACM_0".

if USB_CDC_ACM

config USB_CDC_ACM_RINGBUF_SIZE
	int "USB CDC ACM ring buffer size"
	default 1024
	help
	  USB CDC ACM ring buffer size

config USB_CDC_ACM_DEVICE_NAME
	string "USB CDC ACM device name template"
	default "CDC_ACM"
	help
	  Device name template for the CDC ACM Devices. First device would
	  have name $(USB_CDC_ACM_DEVICE_NAME)_0, etc.

if !USB_CDC_ACM_HAS_DT_COMPAT
module = USB_CDC_ACM
default-count = 1
source "subsys/usb/class/Kconfig.template.composite_device_number"
endif

config CDC_ACM_INTERRUPT_EP_MPS
	int
	default 16
	help
	  CDC ACM class interrupt IN endpoint size

config CDC_ACM_BULK_EP_MPS
	int
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
