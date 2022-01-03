zephyr_library_sources(lorawan_se.c)
add_subdirectory_ifdef(CONFIG_LORAWAN_SE_SETTINGS settings-se)

if (CONFIG_LORAWAN_SE_SETTINGS)
    zephyr_compile_definitions(MBEDTLS_CMAC_C)
endif ()
