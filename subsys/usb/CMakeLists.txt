if(CONFIG_USB_DEVICE_STACK)
  zephyr_sources(
    usb_device.c
    usb_descriptor.c
    )

  add_subdirectory(class)
endif()

if(CONFIG_USB_COMPOSITE_DEVICE)
  zephyr_sources(
    composite.c
    )
endif()

if(CONFIG_USB_DEVICE_VID EQUAL 0xDEAD)
  message(WARNING
    "CONFIG_USB_DEVICE_VID has default test value 0xDEAD.
This value is only for testing and MUST be configured for USB products."
    )
endif()

if(CONFIG_USB_DEVICE_PID EQUAL 0xBEEF)
  message(WARNING
    "CONFIG_USB_DEVICE_PID has default test value 0xBEEF.
This value is only for testing and MUST be configured for USB products."
    )
endif()
