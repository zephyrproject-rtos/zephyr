#
# Copyright (c) 2023, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0
#

set(NRF_BOARD_SELECTED True)

add_subdirectory(${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/nrf5340 nrf5340)

add_subdirectory(.. common)

install(FILES       ${CMAKE_CURRENT_LIST_DIR}/ns/cpuarch_ns.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
        RENAME      cpuarch.cmake)

install(FILES       config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})
