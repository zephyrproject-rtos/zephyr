set_ifndef(BOARD_DEBUG_RUNNER pyocd)
set_ifndef(BOARD_FLASH_RUNNER pyocd)

board_runner_args(pyocd "--target=stm32l562qeixq")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
