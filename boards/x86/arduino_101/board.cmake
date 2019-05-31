# SPDX-License-Identifier: Apache-2.0

if(DEFINED ENV{ZEPHYR_FLASH_OVER_DFU})
  board_set_flasher_ifnset(dfu-util)
endif()

board_set_debugger_ifnset(openocd)

board_runner_args(dfu-util "--pid=8087:0aba" "--alt=x86_app")
board_runner_args(openocd --cmd-pre-load "targets 1")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
