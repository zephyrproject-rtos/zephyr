# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set(NRF_802154_SER_SOURCE_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/nrf_802154_serialization)

zephyr_library_named(nrf_802154_ser)

zephyr_library_sources(
  ${NRF_802154_SER_SOURCE_DIR}/spinel_base/spinel.c
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_buffer_allocator.c
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_buffer_mgr_dst.c
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_buffer_mgr_src.c
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_kvmap.c
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_spinel.c
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_spinel_dec.c
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_serialization_crit_sect.c
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_spinel_log.c
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_spinel_backend_ipc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_spinel_response_notifier.c
)

zephyr_library_sources_ifdef(
  CONFIG_NRF_802154_SER_HOST
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_spinel_app.c
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_spinel_dec_app.c
)

zephyr_library_sources_ifdef(
  CONFIG_NRF_802154_SER_RADIO
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_spinel_net.c
  ${NRF_802154_SER_SOURCE_DIR}/src/nrf_802154_spinel_dec_net.c
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_init_net.c
)

zephyr_include_directories(${NRF_802154_SER_SOURCE_DIR}/include)
zephyr_library_include_directories(${NRF_802154_SER_SOURCE_DIR}/src/include)
