#
# Copyright (c) 2026, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0
#

set(NRF_BOARD_SELECTED True)

add_subdirectory(${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/nrf54lv10a nrf54lv10a)

add_subdirectory(.. common)

install(FILES       ${CMAKE_CURRENT_LIST_DIR}/ns/cpuarch_ns.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
        RENAME      cpuarch.cmake)

install(FILES       config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})

install(DIRECTORY   ${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/nrf54lv10dk_nrf54lv10a_cpuapp/tests

        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)
