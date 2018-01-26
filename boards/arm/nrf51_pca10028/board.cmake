board_runner_args(nrfjprog "--nrf-family=NRF51")
board_runner_args(jlink "--device=nrf51" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
