# SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USBD_VIDEO_CLASS OR CONFIG_USBH_VIDEO_CLASS)
  zephyr_include_directories(.)
  zephyr_sources(uvc.c)
endif()
