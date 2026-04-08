# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Silicon Laboratories Inc.

zephyr_library_sources_ifdef(CONFIG_WIFI_SILABS_SIWX91X
  siwx91x_wifi.c
  siwx91x_wifi_data.c
  siwx91x_wifi_sta.c
  siwx91x_wifi_scan.c)


if(CONFIG_SIWX91X_NWP_RELOCATION_SRAM)
  zephyr_code_relocate(FILES siwx91x_wifi_data.c LOCATION RAM)
endif()
