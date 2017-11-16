set(POST_VERIFY atsamv gpnvm set 1)
board_runner_args(openocd "--cmd-post-verify=\"${POST_VERIFY}\"")
include($ENV{ZEPHYR_BASE}/boards/common/openocd.board.cmake)
