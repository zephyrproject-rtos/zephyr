# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(jlink "--device=STM32F303K8" "--speed=4000")
board_runner_args(pyocd "--target=stm32f303k8tx")
board_runner_args(pyocd "--flash-opt=-O reset_type=hw")
board_runner_args(pyocd "--flash-opt=-O connect_mode=under-reset")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
