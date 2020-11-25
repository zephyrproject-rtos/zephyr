# Copyright (c) 2020 PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_USBH_SAM0  usbh_hcd_atsamx.c)
