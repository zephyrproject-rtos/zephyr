# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_USB_DEVICE_STACK device)
add_subdirectory_ifdef(CONFIG_USB_DEVICE_STACK_NEXT device_next)
add_subdirectory_ifdef(CONFIG_USB_HOST_STACK host)
add_subdirectory_ifdef(CONFIG_USBC_STACK usb_c)
