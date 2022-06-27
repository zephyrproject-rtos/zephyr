# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USB_DEVICE_DRIVER)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USB_DW           usb_dc_dw.c)
zephyr_library_sources_ifdef(CONFIG_USB_DC_RPI_PICO  usb_dc_rpi_pico.c)
zephyr_library_sources_ifdef(CONFIG_USB_DC_STM32     usb_dc_stm32.c)
zephyr_library_sources_ifdef(CONFIG_USB_DC_SAM0      usb_dc_sam0.c)
zephyr_library_sources_ifdef(CONFIG_USB_DC_SAM_USBC  usb_dc_sam_usbc.c)
zephyr_library_sources_ifdef(CONFIG_USB_DC_SAM_USBHS usb_dc_sam_usbhs.c)
zephyr_library_sources_ifdef(CONFIG_USB_KINETIS      usb_dc_kinetis.c)
zephyr_library_sources_ifdef(CONFIG_USB_NATIVE_POSIX
	usb_dc_native_posix.c
	usb_dc_native_posix_adapt.c
	)
zephyr_library_sources_ifdef(CONFIG_USB_NRFX         usb_dc_nrfx.c)
zephyr_library_sources_ifdef(CONFIG_USB_MCUX         usb_dc_mcux.c)

endif()
