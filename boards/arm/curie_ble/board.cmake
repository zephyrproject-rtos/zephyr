board_runner_args(dfu-util "--pid=8087:0aba" "--alt=ble_core")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
