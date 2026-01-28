if(CONFIG_SOC_SERIES_RW6XX)
    if(CONFIG_NET_L2_OPENTHREAD OR CONFIG_NXP_NBU)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.ble ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.ot ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.hdlc ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.coex ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.rw61x ON)

        zephyr_compile_definitions(gPlatformDisableVendorSpecificInit=1U)

        if(CONFIG_NXP_MONOLITHIC_WIFI OR CONFIG_NXP_MONOLITHIC_NBU)
            zephyr_compile_definitions(
                gPlatformMonolithicApp_d=1U
                fw_cpu2_ble=fw_cpu2
                fw_cpu2_combo=fw_cpu2
            )

            zephyr_compile_definitions_ifndef(CONFIG_NXP_MONOLITHIC_NBU
                                            BLE_FW_ADDRESS=0U
                                            COMBO_FW_ADDRESS=0U)

            zephyr_compile_definitions_ifndef(CONFIG_NXP_MONOLITHIC_WIFI
                                            WIFI_FW_ADDRESS=0U)
        endif()
    endif()
endif()

if(CONFIG_SOC_SERIES_MCXW7XX)
    if(CONFIG_NET_L2_OPENTHREAD OR CONFIG_NXP_NBU)
        set(CONFIG_MCUX_COMPONENT_driver.spc ON)
        set(CONFIG_USE_component_osa_zephyr ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.function_lib ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.function_lib.use_toolchain_mem_function ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.ics ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.ble ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.ot ON)
        set_variable_ifdef(CONFIG_SOC_MCXW727C_CPU0 CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.kw47_mcxw72)
        set_variable_ifdef(CONFIG_SOC_MCXW716C CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.kw45_k32w1_mcxw71)
    endif()
endif()

if(CONFIG_SOC_SERIES_MCXW2XX)
    if(CONFIG_BT)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.mcxw23 ON)
        set(CONFIG_MCUX_COMPONENT_middleware.wireless.framework.platform.ble ON)
    endif()
endif()

if(CONFIG_MCUX_COMPONENT_middleware.wireless.framework)
    message(STATUS "connectivity_framework middleware is included.")
    add_subdirectory(${MCUX_SDK_NG_DIR}/middleware/mcuxsdk-middleware-connectivity-framework
        ${CMAKE_CURRENT_BINARY_DIR}/mcuxsdk-middleware-connectivity-framework
    )
endif()
