# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(wifi_ncp.c wifi_ncp_sta.c wifi_ncp_supp.c)
zephyr_library_link_libraries(hostap)
# Interface with the network processor, from the Everest extension of
# WiseConnect
zephyr_library_include_directories(
  ${ZEPHYR_HAL_SILABS_MODULE_DIR}/wiseconnect/extensions/wiseconnect_internal_everest/components/device/silabs/siwx3xx/wireless/interface/wifi_management_interface/wifi/inc/user_command
)
