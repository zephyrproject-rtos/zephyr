#
# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)
zephyr_include_directories(./port/zephyr)
zephyr_sources_ifdef(CONFIG_USB_DC_NXP_EHCI  usb_device_ehci.c)
zephyr_sources_ifdef(CONFIG_USB_DC_NXP_KHCI  usb_device_khci.c)
