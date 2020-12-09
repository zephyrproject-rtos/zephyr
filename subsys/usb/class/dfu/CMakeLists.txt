# SPDX-License-Identifier: Apache-2.0

zephyr_sources(usb_dfu.c)

if(CONFIG_USB_DEVICE_VID EQUAL CONFIG_USB_DEVICE_DFU_VID)
  if(CONFIG_USB_DEVICE_PID EQUAL CONFIG_USB_DEVICE_DFU_PID)
    message(WARNING
      "Run-Time and DFU Mode VID and PID are equal, which can lead to incorrect \
      behavior on some operating systems; see DFU 1.1 Specification Ch. 2 for \
      more details."
      )
  endif()
endif()

if(CONFIG_USB_DEVICE_DFU_VID EQUAL 0x2FE3)
  message(WARNING
    "CONFIG_USB_DEVICE_DFU_VID has default value 0x2FE3.
This value is only for testing and MUST be configured for USB products."
    )

  if(CONFIG_USB_DEVICE_DFU_PID EQUAL 0x101)
    message(WARNING
      "CONFIG_USB_DEVICE_DFU_PID has default value 0x101.
This value is only for testing and MUST be configured for USB products."
      )
  endif()

endif()
