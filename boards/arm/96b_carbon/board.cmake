board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")

include($ENV{ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
