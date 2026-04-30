# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NXP_NEUTRON)
    set(hal_blobs_dir ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs)

    zephyr_include_directories(
        ${hal_blobs_dir}/neutron/common/include
        ${hal_blobs_dir}/neutron/driver/include
    )

    if(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
        zephyr_link_libraries(
            ${hal_blobs_dir}/neutron/rt700/cm33/libNeutronDriver.a
            ${hal_blobs_dir}/neutron/rt700/cm33/libNeutronFirmware.a
        )
    endif()

    if(CONFIG_SOC_MCXN947)
        zephyr_link_libraries(
            ${hal_blobs_dir}/neutron/mcxn/libNeutronDriver.a
            ${hal_blobs_dir}/neutron/mcxn/libNeutronFirmware.a
        )
    endif()
endif()
