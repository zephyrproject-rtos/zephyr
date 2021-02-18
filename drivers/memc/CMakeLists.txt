# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MEMC_STM32 memc_stm32.c)
zephyr_sources_ifdef(CONFIG_MEMC_STM32_SDRAM memc_stm32_sdram.c)
zephyr_linker_sources_ifdef(CONFIG_MEMC_STM32_SDRAM SECTIONS memc_stm32_sdram.ld)

zephyr_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI memc_mcux_flexspi.c)
zephyr_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI_HYPERRAM memc_mcux_flexspi_hyperram.c)

if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
  zephyr_code_relocate(memc_mcux_flexspi.c ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
endif()
