# Copyright (c) 2026 Leica Geosystems AG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USB_VBUS_DETECT_GPIO usb_vbus_detect_gpio.c)
