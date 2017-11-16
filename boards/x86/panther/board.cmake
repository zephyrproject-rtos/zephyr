set(PRE_LOAD targets 1)
board_runner_args(openocd "--cmd-pre-load=\"${PRE_LOAD}\"")
set(OPENOCD_ADDRESS ${CONFIG_PHYS_LOAD_ADDR})
include($ENV{ZEPHYR_BASE}/boards/common/openocd.board.cmake)
