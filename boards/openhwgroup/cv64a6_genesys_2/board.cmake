# Copyright 2024 CISPA Helmholtz Center for Information Security gGmbH
# SPDX-License-Identifier: Apache-2.0
board_runner_args(openocd "--config=${BOARD_DIR}/support/ariane.cfg")
board_runner_args(openocd "--use-elf")
board_runner_args(openocd "--verify")
board_runner_args(openocd "--cmd-pre-init=riscv.cpu configure -work-area-phys 0x90000000 -work-area-size 16780000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
