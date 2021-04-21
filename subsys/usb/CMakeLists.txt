# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USB_DEVICE_STACK)
  zephyr_include_directories(${ZEPHYR_BASE}/subsys/usb)

  zephyr_sources(
    usb_device.c
    usb_descriptor.c
    usb_transfer.c
    )

  add_subdirectory(class)
endif()

zephyr_sources_ifdef(CONFIG_USB_DEVICE_BOS bos.c)

zephyr_sources_ifdef(CONFIG_USB_DEVICE_OS_DESC os_desc.c)

zephyr_sources_ifdef(CONFIG_USB_WORKQUEUE usb_work_q.c)

if(CONFIG_USB_DEVICE_VID EQUAL 0x2FE3)
  message(WARNING
    "CONFIG_USB_DEVICE_VID has default value 0x2FE3.
This value is only for testing and MUST be configured for USB products."
    )

  if(CONFIG_USB_DEVICE_PID EQUAL 0x100)
    message(WARNING
      "CONFIG_USB_DEVICE_PID has default value 0x100.
This value is only for testing and MUST be configured for USB products."
      )
  endif()

endif()
