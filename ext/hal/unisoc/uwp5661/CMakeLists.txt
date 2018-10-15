zephyr_include_directories(
		drivers/include
		)
zephyr_sources(soc/cache.c)
zephyr_sources(soc/system.c)
zephyr_sources(soc/gnss.c)
zephyr_sources(soc/load_fw.c)
zephyr_sources_ifdef(CONFIG_USE_UWP_HAL_UART drivers/src/hal_uart.c)
