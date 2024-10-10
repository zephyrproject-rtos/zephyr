# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

board_runner_args(pyocd "--target=stm32u083rctx")

board_runner_args(jlink "--device=STM32U083RC" "--reset-after-load")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
