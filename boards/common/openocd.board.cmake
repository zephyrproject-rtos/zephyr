# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(openocd)
board_set_debugger_ifnset(openocd)

# "load_image" or "flash write_image erase"?
if(CONFIG_X86 OR CONFIG_ARC)
  set_ifndef(OPENOCD_USE_LOAD_IMAGE YES)
endif()
if(OPENOCD_USE_LOAD_IMAGE)
  set_ifndef(OPENOCD_FLASH load_image)
else()
  set_ifndef(OPENOCD_FLASH "flash write_image erase")
endif()

# SoC specific mass erase options
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

set(OPENOCD_CMD_LOAD_DEFAULT "${OPENOCD_FLASH}")
set(OPENOCD_CMD_VERIFY_DEFAULT "verify_image")

board_finalize_runner_args(openocd
  --cmd-load "${OPENOCD_CMD_LOAD_DEFAULT}"
  --cmd-verify "${OPENOCD_CMD_VERIFY_DEFAULT}"
  )
