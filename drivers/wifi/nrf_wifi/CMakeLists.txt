#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_named(nrf_wifi)

set(OS_AGNOSTIC_BASE ${ZEPHYR_NRF_WIFI_MODULE_DIR})
set(FW_BINS_BASE ${ZEPHYR_NRF_WIFI_MODULE_DIR}/zephyr/blobs/wifi_fw_bins)

zephyr_include_directories(
  inc
  # for net_sprint_ll_addr
  ${ZEPHYR_BASE}/subsys/net/ip
)

zephyr_include_directories_ifdef(CONFIG_NRF70_OFFLOADED_RAW_TX
  off_raw_tx/inc
)

zephyr_library_sources_ifdef(CONFIG_NRF70_SR_COEX
  src/coex.c
)

zephyr_library_sources_ifndef(CONFIG_NRF70_OFFLOADED_RAW_TX
  src/fmac_main.c
)

zephyr_library_sources_ifdef(CONFIG_NRF_WIFI_PATCHES_BUILTIN
  src/fw_load.c
)

if(NOT CONFIG_NRF70_RADIO_TEST AND NOT CONFIG_NRF70_OFFLOADED_RAW_TX)
  zephyr_library_sources(
  src/net_if.c
)
endif()

zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_MGMT
  src/wifi_mgmt_scan.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_SYSTEM_MODE
  src/wifi_mgmt.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_OFFLOADED_RAW_TX
  off_raw_tx/src/off_raw_tx_api.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_STA_MODE
  src/wpa_supp_if.c
  src/wifi_mgmt.c
)

# Without WPA supplicant we only support scan
zephyr_library_sources_ifdef(CONFIG_NRF70_STA_MODE
  src/wpa_supp_if.c)

zephyr_library_sources_ifdef(CONFIG_NRF70_UTIL
  src/wifi_util.c
)

if(CONFIG_NRF70_UTIL OR CONFIG_NRF70_DEBUG_SHELL)
  zephyr_library_sources(src/shell.c)
endif()

zephyr_library_sources_ifdef(CONFIG_NRF70_DEBUG_SHELL
  src/debug_shell.c
)

zephyr_library_link_libraries(nrf70-buslib nrf-wifi-shim)

if (CONFIG_NRF_WIFI_PATCHES_BUILTIN)
  zephyr_blobs_verify(MODULE nrf_wifi REQUIRED)
  # RPU FW patch binaries based on the selected configuration
  if(CONFIG_NRF70_SYSTEM_MODE)
    if (CONFIG_NRF70_SYSTEM_WITH_RAW_MODES)
      set(NRF70_PATCH ${FW_BINS_BASE}/system_with_raw/nrf70.bin)
    else()
      set(NRF70_PATCH ${FW_BINS_BASE}/default/nrf70.bin)
    endif()
  elseif(CONFIG_NRF70_RADIO_TEST)
    set(NRF70_PATCH ${FW_BINS_BASE}/radio_test/nrf70.bin)
  elseif(CONFIG_NRF70_SCAN_ONLY)
    set(NRF70_PATCH ${FW_BINS_BASE}/scan_only/nrf70.bin)
  elseif(CONFIG_NRF70_OFFLOADED_RAW_TX)
    set(NRF70_PATCH ${FW_BINS_BASE}/offloaded_raw_tx/nrf70.bin)
  else()
    # Error
    message(FATAL_ERROR "Unsupported nRF70 patch configuration")
  endif()

  set(gen_inc_dir ${ZEPHYR_BINARY_DIR}/misc/generated)
  zephyr_include_directories(${gen_inc_dir})
  set(gen_dir ${gen_inc_dir}/nrf70_fw_patch)
  generate_inc_file_for_target(
    nrf_wifi
    ${NRF70_PATCH}
    ${gen_dir}/nrf70.bin.inc
  )
endif()
