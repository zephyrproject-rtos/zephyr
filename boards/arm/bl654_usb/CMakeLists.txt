# Copyright (c) 2021 Laird Connectivity.
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_BOARD_BL654_USB AND CONFIG_BOARD_ENABLE_USB_AUTOMATICALLY)
zephyr_library()
zephyr_library_sources(bl654_usb_enable_usb.c)
endif()
