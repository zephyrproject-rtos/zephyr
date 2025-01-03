# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_SERIES_STM32L0X OR CONFIG_SOC_SERIES_STM32L1X)
  board_runner_args(openocd "--cmd-erase=stm32l1x mass_erase 0")
elseif(CONFIG_SOC_SERIES_STM32L4X OR
       CONFIG_SOC_SERIES_STM32L5X OR
       CONFIG_SOC_SERIES_STM32U5X OR
       CONFIG_SOC_SERIES_STM32WBX OR
       CONFIG_SOC_SERIES_STM32G0X OR
       CONFIG_SOC_SERIES_STM32G4X)
  board_runner_args(openocd "--cmd-erase=stm32l4x mass_erase 0")
elseif(CONFIG_SOC_SERIES_STM32F0X OR
       CONFIG_SOC_SERIES_STM32F1X OR
       CONFIG_SOC_SERIES_STM32F3X)
  board_runner_args(openocd "--cmd-erase=stm32f1x mass_erase 0")
elseif(CONFIG_SOC_SERIES_STM32F2X OR
       CONFIG_SOC_SERIES_STM32F4X OR
       CONFIG_SOC_SERIES_STM32F7X)
  board_runner_args(openocd "--cmd-erase=stm32f2x mass_erase 0")
endif()
