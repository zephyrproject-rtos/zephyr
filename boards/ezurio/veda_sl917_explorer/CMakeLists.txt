zephyr_library()
zephyr_library_sources(src/rsi_d_cache.c)
zephyr_library_include_directories(include)

target_compile_definitions(zephyr_interface INTERFACE
            SL_SI91X_ACX_MODULE
            SI91X_32kHz_EXTERNAL_OSCILLATOR)

if(DEFINED CONFIG_SIWX917_PSRAM_PRESENT)
target_compile_definitions(zephyr_interface INTERFACE
            SLI_SI91X_MCU_EXTERNAL_LDO_FOR_PSRAM
            SLI_SI91X_MCU_PSRAM_PRESENT
)
endif()
