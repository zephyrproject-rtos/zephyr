# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MEMC_STM32 memc_stm32.c)
zephyr_sources_ifdef(CONFIG_MEMC_STM32_SDRAM memc_stm32_sdram.c)
zephyr_linker_sources_ifdef(CONFIG_MEMC_STM32_SDRAM SECTIONS memc_stm32_sdram.ld)

zephyr_sources_ifdef(CONFIG_MEMC_MCUX_FLEXSPI memc_mcux_flexspi.c)
