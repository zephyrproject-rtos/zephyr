# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=nrf51822")
board_runner_args(jlink "--device=nRF51822_xxAA" "--speed=4000")
set(OPENOCD_NRF5_SUBFAMILY "nrf51")
# Note: micro:bit DAPLink may be upgraded to J-Link OB by following the instructions at
# https://www.segger.com/products/debug-probes/j-link/models/other-j-links/bbc-microbit-j-link-upgrade/
# in which case the following line should be removed to default back to "jlink" OpenOCD interface
set(OPENOCD_NRF5_INTERFACE "cmsis-dap")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-nrf5.board.cmake)
