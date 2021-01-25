# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set(NRF_802154_SL_ROOT ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/nrf_802154_sl_opensource)

include(${NRF_802154_SL_ROOT}/nrf_802154_sl_files.cmake)

zephyr_library()

zephyr_library_sources(
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_clock_zephyr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_irq_zephyr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_gpiote_zephyr.c
)

if (CONFIG_NRF_802154_SL_OPENSOURCE)
  zephyr_include_directories(${NRF_802154_SL_OPENSOURCE_INCLUDE_DIRS})
  zephyr_library_sources(${NRF_802154_SL_OPENSOURCE_SOURCES})
else ()
  zephyr_include_directories(${NRF_802154_SL_INCLUDE_DIRS})
  zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF52X ${NRF_802154_SL_SOURCES_NRF52})
  zephyr_library_sources_ifdef(
    CONFIG_SOC_SERIES_NRF53X
    ${NRF_802154_SL_SOURCES_NRF53}
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_lp_timer_zephyr.c
  )
endif ()
