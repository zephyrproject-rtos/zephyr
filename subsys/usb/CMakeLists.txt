if(CONFIG_USB_DEVICE_STACK)
  zephyr_sources(
    usb_device.c
    usb_descriptor.c
    )

  add_subdirectory(class)
endif()

if(CONFIG_USB_DEVICE_BOS)
  zephyr_sources(
    bos.c
    )
endif()

if(CONFIG_USB_DEVICE_OS_DESC)
  zephyr_sources(
    os_desc.c
    )
endif()

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
