#
# Copyright (c) 2025, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0
#

set(NRF_BOARD_SELECTED True)

add_subdirectory(${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/nrf7120 nrf7120)

add_subdirectory(.. common)

cmake_path(SET NRF71_SOC_DIR NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/../../../../soc/nordic/nrf71)
target_include_directories(platform_s PRIVATE ${NRF71_SOC_DIR})
target_sources(platform_s PRIVATE ${NRF71_SOC_DIR}/wicr_setup.c)

install(FILES       ${CMAKE_CURRENT_LIST_DIR}/ns/cpuarch_ns.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
        RENAME      cpuarch.cmake)

install(FILES       config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})

install(DIRECTORY   ${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/nrf7120dk_nrf7120_cpuapp/tests
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)
