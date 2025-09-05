# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library_sources_ifdef(CONFIG_USBD_VIDEO_CLASS usb_common_uvc.c)
zephyr_library_sources_ifdef(CONFIG_USBH_VIDEO_CLASS usb_common_uvc.c)
