#
# Copyright (c) 2023, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0
#

set(NRF_BOARD_SELECTED True)

add_subdirectory(${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/nrf91 nrf91)

add_subdirectory(.. common)

install(FILES       ${CMAKE_CURRENT_LIST_DIR}/ns/cpuarch_ns.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
        RENAME      cpuarch.cmake)

install(FILES       ${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/nrf9120/cpuarch.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}/common/nrf9120)

install(FILES       config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})

install(DIRECTORY   ${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/nrf9161dk_nrf9161/tests

        DESTINATION ${INSTALL_PLATFORM_NS_DIR})
