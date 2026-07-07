# Copyright (c) 2025 Conclusive Engineering sp. z o. o.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_TI_CC35XX)
  zephyr_library_sources(
    ti_cc35xx_wifi.c
    )
endif()
