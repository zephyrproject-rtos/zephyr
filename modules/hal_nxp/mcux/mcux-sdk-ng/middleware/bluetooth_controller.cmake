# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NXP_SNPS_BLE_CTRL)
    set(CONFIG_MCUX_COMPONENT_middleware.wireless.snps_ll ON)
    set(CONFIG_MCUX_COMPONENT_middleware.wireless.snps_ll_os ON)

    if(CONFIG_SOC_SERIES_MCXW2XX)
        set(BLE_LIBS_PATH ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/mcxw23)
    endif()

    set(BLELIBOS_PATH       ${BLE_LIBS_PATH}/libble_ll_os.a)
    set(BLELIBUTILS_PATH    ${BLE_LIBS_PATH}/libble_ll_utils_os.a)

    zephyr_compile_definitions(
        gUseHciTransportDownward_d=1
        OSA_USED
    )

    zephyr_blobs_verify(FILES
        ${BLELIBOS_PATH}
        ${BLELIBUTILS_PATH}
    )
    zephyr_library_import(ble_lib_os ${BLELIBOS_PATH})
    zephyr_library_import(ble_lib_utils ${BLELIBUTILS_PATH})

    add_subdirectory(${MCUX_SDK_NG_DIR}/middleware/mcuxsdk-middleware-bluetooth-controller
        ${CMAKE_CURRENT_BINARY_DIR}/mcuxsdk-middleware-bluetooth-controller
    )
endif()
