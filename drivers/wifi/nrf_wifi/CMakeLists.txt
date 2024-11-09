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
  src/qspi/inc
  # for net_sprint_ll_addr
  ${ZEPHYR_BASE}/subsys/net/ip
)

zephyr_include_directories_ifdef(CONFIG_NRF70_OFFLOADED_RAW_TX
  off_raw_tx/inc
)

zephyr_library_sources_ifdef(CONFIG_NRF70_SR_COEX
  src/coex.c
)

zephyr_library_sources(
  src/qspi/src/device.c
  src/qspi/src/rpu_hw_if.c
  src/qspi/src/ficr_prog.c
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

zephyr_library_sources_ifdef(CONFIG_NRF70_SYSTEM_MODE_COMMON
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

zephyr_library_sources_ifdef(CONFIG_NRF70_ON_QSPI
  src/qspi/src/qspi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_ON_SPI
  src/qspi/src/spi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF70_UTIL
  src/wifi_util.c
)

zephyr_compile_definitions_ifdef(CONFIG_NRF70_ON_QSPI
# These are XIP related anomalies and aren't applicable for nRF7002 and cause
# throughput issues.
  -DNRF53_ERRATA_43_ENABLE_WORKAROUND=0
  -DNRF52_ERRATA_215_ENABLE_WORKAROUND=0
# nRF70 QSPI doesn't use 192MHz clock and most samples use 128MHz, this can cause anomaly 159
# but as its rare and not seen in most cases, we can disable it.
# Alternative is 128MHz CPU should be disabled that impacts Wi-Fi performance.
  -DNRF53_ERRATA_159_ENABLE_WORKAROUND=0
)

if (CONFIG_NRF_WIFI_BUILD_ONLY_MODE)
  message(WARNING "
  ------------------------------------------------------------------------
  Building only the nRF70 driver, skipping firmware patch.
  This is only for building (CI) purposes and will not work on a real device.
  ------------------------------------------------------------------------
  ")
elseif(CONFIG_NRF_WIFI_PATCHES_BUILTIN)
  zephyr_blobs_verify(MODULE nrf_wifi REQUIRED)
  # RPU FW patch binaries based on the selected configuration
  if(CONFIG_NRF70_SYSTEM_MODE)
    set(NRF70_PATCH ${FW_BINS_BASE}/default/nrf70.bin)
  elseif(CONFIG_NRF70_RADIO_TEST)
    set(NRF70_PATCH ${FW_BINS_BASE}/radio_test/nrf70.bin)
  elseif(CONFIG_NRF70_SCAN_ONLY)
    set(NRF70_PATCH ${FW_BINS_BASE}/scan_only/nrf70.bin)
  elseif (CONFIG_NRF70_SYSTEM_WITH_RAW_MODES)
    set(NRF70_PATCH ${FW_BINS_BASE}/system_with_raw/nrf70.bin)
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
