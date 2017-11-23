# Inventek ism43362-m3g-l44 WiFi chip
#
# Copyright (c) 2017 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_INVENTEK_WIFI)
  if(CONFIG_WIFI_ISM43362_M3G_L44)
    zephyr_include_directories(Inc)

    zephyr_sources(Src/es_wifi.c)
    zephyr_sources(Src/wifi.c)
  endif()
endif()

