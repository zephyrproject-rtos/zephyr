# Copyright (c) 2025 Linumiz GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_USBH_CDC_ECM_CLASS usb_cdc_ecm.c)
