if(DEFINED ENV{ZEPHYR_FLASH_OVER_DFU})
  set(BOARD_FLASH_RUNNER dfu-util)
endif()

set(BOARD_DEBUG_RUNNER openocd)

board_runner_args(dfu-util "--pid=8087:0aba" "--alt=x86_app")
board_runner_args(openocd --cmd-pre-load "targets 1")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
