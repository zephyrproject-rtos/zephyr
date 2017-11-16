board_runner_args(nios2 "--cpu-sof=$ENV{ZEPHYR_BASE}/arch/nios2/soc/nios2f-zephyr/cpu/ghrd_10m50da.sof")
include($ENV{ZEPHYR_BASE}/boards/common/nios2.board.cmake)
