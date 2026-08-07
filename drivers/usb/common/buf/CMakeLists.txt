# Copyright Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_UDC_DRIVER OR CONFIG_UHC_DRIVER)
  zephyr_library()
  zephyr_library_sources(usb_buf.c)
endif()
