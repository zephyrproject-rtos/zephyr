if(CONFIG_NXP_MULTICORE)
    set(CONFIG_MCUX_COMPONENT_middleware.multicore.mcmgr ON)
    set_variable_ifdef(CONFIG_SOC_MCXW716C      CONFIG_MCUX_COMPONENT_middleware.multicore.mcmgr.mcxw716)
    set_variable_ifdef(CONFIG_SOC_MCXW727C_CPU0 CONFIG_MCUX_COMPONENT_middleware.multicore.mcmgr.mcxw727)

    set(CONFIG_MCUX_COMPONENT_middleware.multicore.rpmsg-lite ON)
    set(CONFIG_MCUX_COMPONENT_middleware.multicore.rpmsg-lite.zephyr ON)
    set_variable_ifdef(CONFIG_SOC_MCXW716C      CONFIG_MCUX_COMPONENT_middleware.multicore.rpmsg-lite.mcxw71x)
    set_variable_ifdef(CONFIG_SOC_MCXW727C_CPU0 CONFIG_MCUX_COMPONENT_middleware.multicore.rpmsg-lite.mcxw72x)

    add_subdirectory(${MCUX_SDK_NG_DIR}/middleware/mcuxsdk-middleware-multicore
        ${CMAKE_CURRENT_BINARY_DIR}/mcuxsdk-middleware-multicore
    )
endif()
