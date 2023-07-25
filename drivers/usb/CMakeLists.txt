# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_UDC_DRIVER udc)
add_subdirectory_ifdef(CONFIG_UHC_DRIVER uhc)
add_subdirectory_ifdef(CONFIG_UVB uvb)
add_subdirectory_ifdef(CONFIG_USB_BC12 bc12)
add_subdirectory_ifdef(CONFIG_USB_DEVICE_DRIVER device)
