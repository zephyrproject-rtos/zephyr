board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

board_runner_args(pyocd "--target=stm32u5a5zjtx")

board_runner_args(jlink "--device=STM32U5A5ZJ" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
