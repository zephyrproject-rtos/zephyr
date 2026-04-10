# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Silicon Laboratories Inc.

zephyr_library_sources_ifdef(CONFIG_WIFI_SILABS_SIWX91X
  siwx91x_wifi.c
  siwx91x_wifi_ap.c
  siwx91x_wifi_ps.c
  siwx91x_wifi_sta.c
  siwx91x_wifi_scan.c
  siwx91x_wifi_reg.c)

zephyr_library_sources_ifdef(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD
  siwx91x_wifi_socket.c)

zephyr_library_sources_ifdef(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE
  siwx91x_wifi_data.c)

if(CONFIG_SIWX91X_NWP_RELOCATION_SRAM AND CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE)
  zephyr_code_relocate(FILES siwx91x_wifi_data.c LOCATION RAM)
endif()
