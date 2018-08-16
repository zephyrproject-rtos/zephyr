if(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
  target_include_directories(drivers PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth)
endif()

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_SPI_NOR}              spi_nor.c
            IFDEF:${CONFIG_SPI_FLASH_W25QXXDV}   spi_flash_w25qxxdv.c
            IFDEF:${CONFIG_SOC_FLASH_QMSI}       soc_flash_qmsi.c
            IFDEF:${CONFIG_SOC_FLASH_NRF}        soc_flash_nrf.c
            IFDEF:${CONFIG_SOC_FLASH_MCUX}       soc_flash_mcux.c
            IFDEF:${CONFIG_FLASH_PAGE_LAYOUT}    flash_page_layout.c
            IFDEF:${CONFIG_USERSPACE}            flash_handlers.c
            IFDEF:${CONFIG_SOC_FLASH_SAM0}       flash_sam0.c
            IFDEF:${CONFIG_SOC_FLASH_NIOS2_QSPI} soc_flash_nios2_qspi.c
            IFDEF:${CONFIG_SOC_FLASH_GECKO}      flash_gecko.c
            IFDEF:${CONFIG_FLASH_SHELL}          flash_shell.c

)

if(CONFIG_SOC_FLASH_STM32)
  zephyr_list(SOURCES APPEND
              OUTPUT PRIVATE_SOURCES
              IFDEF:${CONFIG_SOC_SERIES_STM32F0X} flash_stm32.c
                                                  flash_stm32f0x.c
              IFDEF:${CONFIG_SOC_SERIES_STM32F3X} flash_stm32f3x.c
                                                  flash_stm32f3x_priv.c
              IFDEF:${CONFIG_SOC_SERIES_STM32L4X} flash_stm32.c
                                                  flash_stm32l4x.c
              IFDEF:${CONFIG_SOC_SERIES_STM32F4X} flash_stm32.c
                                                  flash_stm32f4x.c
  )
endif()

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
