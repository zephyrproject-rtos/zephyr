# Copyright (c) 2026 Davi Alexandre Volcan <davi.volcan2@gmail.com>
# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=sw")
board_runner_args(jlink "--device=STM32H743II" "--speed=4000")
board_runner_args(pyocd "--target=stm32h743iitx")

# keep first
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
