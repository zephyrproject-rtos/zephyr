# Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-pre-init "source [find target/bl702l.cfg]")

board_runner_args(openocd --file-type=elf --no-load)
board_runner_args(openocd --gdb-init "set print asm-demangle on")
board_runner_args(openocd --gdb-init "mem 0x21000000 0x21040000 ro")
board_runner_args(openocd --gdb-init "mem 0x23000000 0x24000000 ro")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(bflb_mcu_tool --chipname bl702l)
include(${ZEPHYR_BASE}/boards/common/bflb_mcu_tool.board.cmake)

board_set_flasher(bflb_mcu_tool)
