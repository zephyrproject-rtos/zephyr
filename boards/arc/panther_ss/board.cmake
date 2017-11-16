set(PRE_LOAD targets 1)
board_runner_args(openocd "--cmd-pre-load=\"${PRE_LOAD}\"")
set(OPENOCD_USE_LOAD_IMAGE NO)
include($ENV{ZEPHYR_BASE}/boards/common/openocd.board.cmake)
