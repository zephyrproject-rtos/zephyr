board_runner_args(nrfjprog "--nrf-family=NRF52")
board_runner_args(jlink "--device=nrf52" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
