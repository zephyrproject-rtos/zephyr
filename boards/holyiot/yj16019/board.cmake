board_runner_args(nrfjprog "--softreset")
board_runner_args(jlink "--device=nRF52832_xxAA" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
