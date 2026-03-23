# Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_UDC_SF32LB)
  zephyr_library()

  zephyr_library_sources(usb_dc_sf32lb.c)
endif()
