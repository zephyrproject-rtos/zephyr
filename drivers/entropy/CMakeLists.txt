zephyr_library_ifdef(CONFIG_ENTROPY_HAS_DRIVER)

zephyr_library_sources_ifdef(CONFIG_ENTROPY_ESP32_RNG          entropy_esp32.c)
zephyr_library_sources_ifdef(CONFIG_ENTROPY_MCUX_RNGA          entropy_mcux_rnga.c)
zephyr_library_sources_ifdef(CONFIG_ENTROPY_MCUX_TRNG          entropy_mcux_trng.c)
zephyr_library_sources_ifdef(CONFIG_ENTROPY_NRF5_RNG           entropy_nrf5.c)
zephyr_library_sources_ifdef(CONFIG_ENTROPY_STM32_RNG          entropy_stm32.c)
zephyr_library_sources_ifdef(CONFIG_ENTROPY_NATIVE_POSIX       entropy_native_posix.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE                  entropy_handlers.c)
