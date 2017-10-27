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
