#
# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_USB_DC_NXP_EHCI  usb_phy.c)
