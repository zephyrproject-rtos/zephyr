zephyr_sources_ifdef(CONFIG_SPI_FLASH_W25QXXDV spi_flash_w25qxxdv.c)
zephyr_sources_ifdef(CONFIG_SOC_FLASH_QMSI soc_flash_qmsi.c)
zephyr_sources_ifdef(CONFIG_SOC_FLASH_NRF5 soc_flash_nrf5.c)
zephyr_sources_ifdef(CONFIG_SOC_FLASH_MCUX soc_flash_mcux.c)
zephyr_sources_ifdef(CONFIG_FLASH_PAGE_LAYOUT flash_page_layout.c)
zephyr_sources_ifdef(CONFIG_USERSPACE flash_handlers.c)

if(CONFIG_SOC_SERIES_STM32F0X)
zephyr_sources_ifdef(CONFIG_SOC_FLASH_STM32
	flash_stm32.c
	flash_stm32f0x.c
	)
endif()

if(CONFIG_SOC_SERIES_STM32F3X)
zephyr_sources_ifdef(CONFIG_SOC_FLASH_STM32 flash_stm32f3x.c)
zephyr_sources_ifdef(CONFIG_SOC_FLASH_STM32 flash_stm32f3x_priv.c)
endif()

if(CONFIG_SOC_SERIES_STM32F4X)
zephyr_sources_ifdef(CONFIG_SOC_FLASH_STM32
	flash_stm32.c
	flash_stm32f4x.c
	)
endif()

if(CONFIG_SOC_SERIES_STM32L4X)
zephyr_sources_ifdef(CONFIG_SOC_FLASH_STM32
	flash_stm32.c
	flash_stm32l4x.c
	)
endif()

zephyr_include_directories_ifdef(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC $ENV{ZEPHYR_BASE}/subsys/bluetooth)
