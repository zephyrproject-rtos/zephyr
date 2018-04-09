if(DEFINED ENV{ZEPHYR_FLASH_OVER_DFU})
  set(BOARD_FLASH_RUNNER dfu-util)
else()
  set(BOARD_FLASH_RUNNER openocd)
endif()

set(BOARD_DEBUG_RUNNER openocd)

board_runner_args(dfu-util "--pid=8087:0aba" "--alt=sensor_core")
board_runner_args(openocd --cmd-pre-load "targets 1" "--gdb-port=3334")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
