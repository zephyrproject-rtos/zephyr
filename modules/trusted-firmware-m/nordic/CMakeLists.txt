#
# Copyright (c) 2023, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_policy(SET CMP0076 NEW)
set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

set(partition_includes
    ${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/${NRF_SOC_VARIANT}/partition
    ${CMAKE_BINARY_DIR}/../zephyr/include/generated
)

set(board_includes
    ${CMAKE_BINARY_DIR}/../zephyr/misc/generated/syscalls_links/include
    ${ZEPHYR_BASE}/include
)

target_include_directories(platform_region_defs
    INTERFACE
        ${partition_includes}
)


target_sources(platform_s
    PRIVATE
        $<$<BOOL:${TFM_PARTITION_PLATFORM}>:${CMAKE_CURRENT_SOURCE_DIR}/src/tfm_platform_system.c>
)

target_include_directories(platform_s
    PUBLIC
        services/include
        include
        ${partition_includes}
        ${board_includes}
)

if(BL2)
    target_include_directories(platform_bl2
        PUBLIC
            include
            include/util
            ${partition_includes}
            ${board_includes}
    )
endif()

target_sources(tfm_spm
    PRIVATE
        src/tfm_hal_platform.c
)

if (TFM_PARTITION_PLATFORM)
install(FILES       include/tfm_ioctl_api.h
                    include/device_cfg.h
                    include/RTE_Device.h
                    include/tfm_ioctl_api.h
        DESTINATION ${INSTALL_INTERFACE_INC_DIR})
endif()

install(FILES       ns/CMakeLists.txt
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})
