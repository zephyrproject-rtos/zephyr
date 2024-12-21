#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

set(NRF_WIFI_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

if (CONFIG_NRF70_BUSLIB)
  zephyr_library_named(nrf70-buslib)
  zephyr_library_include_directories(
    inc
    ${NRF_WIFI_DIR}/os_if/inc
  )
  zephyr_library_include_directories_ifdef(CONFIG_NRF71_ON_IPC
    ${NRF_WIFI_DIR}/bus_if/bal/inc
    # QSPI is common to (Q)SPI and IPC
    ${NRF_WIFI_DIR}/bus_if/bus/qspi/inc
    ${NRF_WIFI_DIR}/fw_if/umac_if/inc/fw
    ${NRF_WIFI_DIR}/hw_if/hal/inc
  )

  zephyr_library_compile_definitions_ifdef(CONFIG_NRF71_ON_IPC
    NRF71_ON_IPC
  )
  zephyr_library_sources(
    device.c
  )
  if(NOT CONFIG_WIFI_NRF70)
    zephyr_library_sources(
      ${NRF_WIFI_DIR}/os_if/src/osal.c
    )
  endif()
  zephyr_library_sources_ifdef(CONFIG_NRF70_ON_QSPI
    rpu_hw_if.c
    qspi_if.c
  )
  zephyr_library_sources_ifdef(CONFIG_NRF70_ON_SPI
    rpu_hw_if.c
    spi_if.c
  )
  zephyr_library_sources_ifdef(CONFIG_NRF71_ON_IPC
    ipc_if.c
    ipc_service.c
    spsc_qm.c
  )
endif()
