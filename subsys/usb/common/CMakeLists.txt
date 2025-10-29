# Copyright (c) 2025 Nordic Semiconductor ASA
# Copyright (c) 2025 Linumiz GmbH
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USBD_VIDEO_CLASS OR CONFIG_USBH_VIDEO_CLASS)
  zephyr_include_directories(include)
  zephyr_sources(usb_common_uvc.c)
endif()

zephyr_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_USBH_CDC_ECM_CLASS usb_cdc_ecm.c)
