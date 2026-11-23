#
# Copyright (c) 2025, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0
#

set(NRF_BOARD_SELECTED True)

add_subdirectory(${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/nrf54lm20a nrf54lm20a)

add_subdirectory(.. common)

install(FILES       ${CMAKE_CURRENT_LIST_DIR}/ns/cpuarch_ns.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
        RENAME      cpuarch.cmake)

install(FILES       config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})

install(DIRECTORY   ${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/nrf54lm20dk_nrf54lm20a_cpuapp/tests

        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)
