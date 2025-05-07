# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_WIFI_NRF70)
  return()
endif()

add_subdirectory(${ZEPHYR_NRF_WIFI_MODULE_DIR} nrf_wifi_osal)

zephyr_library_named(nrf-wifi-shim)
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR})
zephyr_include_directories(${ZEPHYR_NRF_WIFI_MODULE_DIR}/os_if/inc)
zephyr_library_sources(
  shim.c
  timer.c
  work.c
)
zephyr_include_directories_ifdef(CONFIG_NRF71_ON_IPC
  ${CMAKE_CURRENT_LIST_DIR}/../bus/
  ${CMAKE_CURRENT_LIST_DIR}/../hw_if/hal/inc
)

zephyr_library_compile_definitions_ifdef(CONFIG_NRF71_ON_IPC
  NRF71_ON_IPC
)

zephyr_library_link_libraries(nrf-wifi-osal)
