board_runner_args(dfu-util "--pid=0483:df11" "--alt=0")
board_runner_args(dfu-util "--dfuse-addr=${CONFIG_FLASH_BASE_ADDRESS}")

include($ENV{ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
