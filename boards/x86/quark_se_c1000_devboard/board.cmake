set(PRE_LOAD targets 1)
board_runner_args(openocd "--cmd-pre-load=\"${PRE_LOAD}\"")
include($ENV{ZEPHYR_BASE}/boards/common/openocd.board.cmake)
