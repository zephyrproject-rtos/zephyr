# SPDX-License-Identifier: Apache-2.0

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")

board_runner_args(openocd --cmd-pre-init "source [find interface/stlink.cfg]")
board_runner_args(openocd --cmd-pre-init "source [find target/stm32f0x.cfg]")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
