#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

set(NRF_WIFI_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

if (CONFIG_NRF70_BUSLIB)

  if(CONFIG_NRF70_ON_QSPI OR CONFIG_NORDIC_QSPI_NOR)
    zephyr_compile_definitions(
    # These are XIP related anomalies and aren't applicable for nRF7002 and cause
    # throughput issues.
      -DNRF53_ERRATA_43_ENABLE_WORKAROUND=0
      -DNRF52_ERRATA_215_ENABLE_WORKAROUND=0
    # nRF70 QSPI doesn't use 192MHz clock and most samples use 128MHz, this can cause anomaly 159
    # but as its rare and not seen in most cases, we can disable it.
    # Alternative is 128MHz CPU should be disabled that impacts Wi-Fi performance.
      -DNRF53_ERRATA_159_ENABLE_WORKAROUND=0
    )
  endif()

  zephyr_library_named(nrf70-buslib)
  zephyr_library_include_directories(
    inc
    ${NRF_WIFI_DIR}/os_if/inc
  )
  zephyr_library_sources(
    rpu_hw_if.c
    device.c
  )
  if(NOT CONFIG_WIFI_NRF70)
    zephyr_library_sources(
      ${NRF_WIFI_DIR}/os_if/src/osal.c
    )
  endif()
  zephyr_library_sources_ifdef(CONFIG_NRF70_ON_QSPI
    qspi_if.c
  )
  zephyr_library_sources_ifdef(CONFIG_NRF70_ON_SPI
    spi_if.c
  )
endif()
