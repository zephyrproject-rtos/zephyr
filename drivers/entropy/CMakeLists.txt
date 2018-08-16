zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_ENTROPY_ESP32_RNG}         entropy_esp32.c
            IFDEF:${CONFIG_ENTROPY_MCUX_RNGA}         entropy_mcux_rnga.c
            IFDEF:${CONFIG_ENTROPY_MCUX_TRNG}         entropy_mcux_trng.c
            IFDEF:${CONFIG_ENTROPY_NRF5_RNG}          entropy_nrf5.c
            IFDEF:${CONFIG_ENTROPY_SAM_RNG}            entropy_sam.c
            IFDEF:${CONFIG_ENTROPY_STM32_RNG}         entropy_stm32.c
            IFDEF:${CONFIG_FAKE_ENTROPY_NATIVE_POSIX} fake_entropy_native_posix.c
            IFDEF:${CONFIG_USERSPACE}                 entropy_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
