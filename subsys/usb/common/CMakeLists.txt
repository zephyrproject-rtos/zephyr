# Copyright (c) 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)
zephyr_library_sources_ifdef(CONFIG_USB_COMMON_UVC usb_common_uvc.c)
