zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_USB_DEVICE_STACK}   usb_device.c
                                               usb_descriptor.c
            IFDEF:${CONFIG_USB_DEVICE_BOS}     bos.c
            IFDEF:${CONFIG_USB_DEVICE_OS_DESC} os_desc.c
)

if(PRIVATE_SOURCES)
  add_library(subsys_usb STATIC "")
  zephyr_set_property(TARGET subsys_usb MEMORY_SPACE kernel)
  target_link_libraries(subsys_usb PRIVATE zephyr_interface)
  target_link_libraries(zephyr_app_linking INTERFACE subsys_usb)
  target_sources(subsys_usb PRIVATE ${PRIVATE_SOURCES})
endif()


if(CONFIG_USB_DEVICE_STACK)
  add_subdirectory(class)
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
