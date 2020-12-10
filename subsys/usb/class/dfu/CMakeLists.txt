# SPDX-License-Identifier: Apache-2.0

zephyr_sources(usb_dfu.c)

if(CONFIG_USB_DEVICE_PID EQUAL CONFIG_USB_DEVICE_DFU_PID)
  message(WARNING
    "Run-Time and DFU Mode VID and PID are equal, which can lead to incorrect \
    behavior on some operating systems; see DFU 1.1 Specification Ch. 2 for \
    more details."
    )
endif()
