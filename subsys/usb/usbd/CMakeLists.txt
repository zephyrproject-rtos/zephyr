# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USBD_DEVICE_STACK)
	zephyr_include_directories(${ZEPHYR_BASE}/subsys/usb/usbd)
	zephyr_sources(
		usbd_core.c
		usbd_desc.c
		usbd_buf.c
		usbd_ch9.c
		usbd_api.c
	)
	add_subdirectory(class)
endif()


if(CONFIG_USBD_SHELL)
	zephyr_sources(usbd_shell.c)
endif()

if(CONFIG_USBD_DEVICE_VID EQUAL 0x2FE3)
message(WARNING
"CONFIG_USBD_DEVICE_VID has default value 0x2FE3.
This value is only for testing and MUST be configured for USB products."
)
endif()
