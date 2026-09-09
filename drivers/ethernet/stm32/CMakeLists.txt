# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ETH_STM32_HAL_RX_DESC_COUNT)
  zephyr_compile_definitions(ETH_RX_DESC_CNT=${CONFIG_ETH_STM32_HAL_RX_DESC_COUNT})
endif()

if(CONFIG_STM32_HAL2)
  zephyr_library_sources_ifdef(CONFIG_ETH_STM32_HAL2_API eth_stm32_hal2.c)
else()
  zephyr_library_sources(eth_stm32_hal_common.c)
  zephyr_library_sources_ifdef(CONFIG_ETH_STM32_HAL_API_V1 eth_stm32_hal_v1.c)
  zephyr_library_sources_ifdef(CONFIG_ETH_STM32_HAL_API_V2 eth_stm32_hal_v2.c)
  zephyr_library_sources_ifdef(CONFIG_PTP_CLOCK_STM32_HAL eth_stm32_hal_ptp.c)
endif()
