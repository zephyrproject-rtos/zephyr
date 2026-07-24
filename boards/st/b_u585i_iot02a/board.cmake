# SPDX-License-Identifier: Apache-2.0

# keep first
if(CONFIG_FLASH_STM32_NOR_MEMMAP)
  board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
  board_runner_args(stm32cubeprogrammer "--extload=MX25LM51245G_STM32U585I-IOT02A.stldr")
else()
  board_runner_args(stm32cubeprogrammer "--erase" "--port=swd" "--reset-mode=hw")
endif()

if(CONFIG_BUILD_WITH_TFM)
  board_runner_args(stm32cubeprogrammer "--erase")
endif()

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

board_runner_args(jlink "--device=STM32U585AI" "--reset-after-load")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
# FIXME: openocd runner requires use of STMicro openocd fork.
# Check board documentation for more details.
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/st/common/CMakeLists.txt)
