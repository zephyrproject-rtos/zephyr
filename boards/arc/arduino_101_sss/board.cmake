# SPDX-License-Identifier: Apache-2.0

if(DEFINED ENV{ZEPHYR_FLASH_OVER_DFU})
  board_set_flasher_ifnset(dfu-util)
else()
  board_set_flasher_ifnset(openocd)
endif()

board_set_debugger_ifnset(openocd)

board_runner_args(dfu-util "--pid=8087:0aba" "--alt=sensor_core")
board_runner_args(openocd --cmd-pre-load "targets 1" "--gdb-port=3334")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
