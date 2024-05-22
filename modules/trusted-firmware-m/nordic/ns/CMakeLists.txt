#
# Copyright (c) 2023, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_policy(SET CMP0076 NEW)
set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(NRF_BOARD_SELECTED True)

add_library(platform_ns STATIC)

set(partition_includes
    ${CMAKE_CURRENT_LIST_DIR}/common/${NRF_SOC_VARIANT}/partition
    ${CMAKE_BINARY_DIR}/../zephyr/include/generated
)

set(board_includes
    ${CMAKE_BINARY_DIR}/../zephyr/misc/generated/syscalls_links/include
)

target_include_directories(platform_region_defs
    INTERFACE
        ${partition_includes}
)

target_include_directories(platform_ns
    PUBLIC
        ${partition_includes}
        ${board_includes}
)

# Get the value of HAL_NORDIC_PATH
include(${CMAKE_CURRENT_LIST_DIR}/common/core/config_nordic_nrf_spe.cmake)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/common/${NRF_SOC_VARIANT} ${NRF_SOC_VARIANT})

target_include_directories(platform_ns
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(platform_ns
    PUBLIC
        platform_region_defs
)
