# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_CC13XX_CC26XX soc_flash_cc13xx_cc26xx.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_TELINK_B91 soc_flash_b91.c)
zephyr_library_sources_ifdef(CONFIG_SPI_NOR spi_nor.c)
zephyr_library_sources_ifdef(CONFIG_NORDIC_QSPI_NOR nrf_qspi_nor.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_SIMULATOR flash_simulator.c)
zephyr_library_sources_ifdef(CONFIG_SPI_FLASH_AT45 spi_flash_at45.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_ITE_IT8XXX2 flash_ite_it8xxx2.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF soc_flash_nrf.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_TICKER soc_flash_nrf_ticker.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_MCUX soc_flash_mcux.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_LPC soc_flash_lpc.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_PAGE_LAYOUT flash_page_layout.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE flash_handlers.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_SAM0 flash_sam0.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_SAM flash_sam.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NIOS2_QSPI soc_flash_nios2_qspi.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_GECKO flash_gecko.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_RV32M1 soc_flash_rv32m1.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_STM32_QSPI flash_stm32_qspi.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_STM32_OSPI flash_stm32_ospi.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_MX25UM51345G flash_mcux_flexspi_mx25um51345g.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_NOR flash_mcux_flexspi_nor.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_MCUX_FLEXSPI_HYPERFLASH flash_mcux_flexspi_hyperflash.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_ESP32 flash_esp32.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_SMARTBOND flash_smartbond.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_CAD_QSPI_NOR flash_cadence_qspi_nor.c flash_cadence_qspi_nor_ll.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_XMC4XXX soc_flash_xmc4xxx.c)

if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
  dt_chosen(chosen_flash PROPERTY "zephyr,flash")
  dt_prop(compat_flash PATH ${chosen_flash} PROPERTY compatible)
  if(compat_flash MATCHES "nxp,imx-flexspi-nor")
	  zephyr_code_relocate(flash_mcux_flexspi_nor.c ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  elseif(compat_flash MATCHES "nxp,imx-flexspi-mx25um51345g")
	  zephyr_code_relocate(flash_mcux_flexspi_mx25um51345g.c ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  elseif(compat_flash MATCHES "nxp,imx-flexspi-hyperflash")
    zephyr_code_relocate(flash_mcux_flexspi_hyperflash.c ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  endif()
endif()

if(CONFIG_SOC_FLASH_STM32)
  if(CONFIG_SOC_SERIES_STM32H7X)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32H7X flash_stm32h7x.c)
  else()
    zephyr_library_sources(flash_stm32.c)

    zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_STM32_V1 flash_stm32_v1.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32F2X flash_stm32f2x.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32F4X flash_stm32f4x.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32F7X flash_stm32f7x.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32L4X flash_stm32l4x.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32L5X flash_stm32l5_u5.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32WLX flash_stm32l4x.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32WBX flash_stm32wbx.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32G0X flash_stm32g0x.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32G4X flash_stm32g4x.c)
    zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_STM32U5X flash_stm32l5_u5.c)
  endif()
endif()

if(CONFIG_SOC_FLASH_GD32)
  zephyr_library_sources(flash_gd32.c)

  zephyr_library_sources_ifdef(CONFIG_GD32_NV_FLASH_V1 flash_gd32_v1.c)
  zephyr_library_sources_ifdef(CONFIG_GD32_NV_FLASH_V2 flash_gd32_v2.c)
  zephyr_library_sources_ifdef(CONFIG_GD32_NV_FLASH_V3 flash_gd32_v3.c)
endif()

zephyr_library_include_directories_ifdef(
  CONFIG_FLASH_MCUX_FLEXSPI_NOR
  ${ZEPHYR_BASE}/drivers/memc
)

zephyr_library_include_directories_ifdef(
  CONFIG_FLASH_MCUX_FLEXSPI_MX25UM51345G
  ${ZEPHYR_BASE}/drivers/memc
)

zephyr_library_include_directories_ifdef(
  CONFIG_FLASH_MCUX_FLEXSPI_HYPERFLASH
  ${ZEPHYR_BASE}/drivers/memc
)

zephyr_library_include_directories_ifdef(
	CONFIG_SOC_FLASH_NRF_RADIO_SYNC_TICKER
	${ZEPHYR_BASE}/subsys/bluetooth
	${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
	)

zephyr_library_sources_ifdef(CONFIG_FLASH_SHELL flash_shell.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_JESD216 jesd216.c)
