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

# Extra OpenOCD scripts search path for STM32MP boards.
#
# This is useful when using a custom OpenOCD build that has not been
# installed system-wide (e.g. built in-place from a git checkout): in that
# case OpenOCD's compiled-in default scripts path does not exist, and its
# own tcl/ scripts directory (containing board/st/... and target/st/...)
# must be added to the search path explicitly, in addition to pointing
# -DOPENOCD at the built binary.
set(STM32MP_OPENOCD_SCRIPTS "" CACHE PATH
  "Extra OpenOCD scripts search path, e.g. the tcl/ directory of a \
non-installed OpenOCD build")

if(STM32MP_OPENOCD_SCRIPTS)
  set(OPENOCD_DEFAULT_PATH ${STM32MP_OPENOCD_SCRIPTS})
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
