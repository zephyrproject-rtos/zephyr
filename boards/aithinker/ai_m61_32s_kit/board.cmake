# Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-pre-init "source [find bl61x.cfg]")

board_runner_args(openocd --use-elf --no-load --no-init)
board_runner_args(openocd --gdb-init "set mem inaccessible-by-default off")
board_runner_args(openocd --gdb-init "set architecture riscv:rv32")
board_runner_args(openocd --gdb-init "set remotetimeout 250")
board_runner_args(openocd --gdb-init "set print asm-demangle on")
board_runner_args(openocd --gdb-init "set backtrace limit 32")
board_runner_args(openocd --gdb-init "mem 0x22FC0000 0x23010000 rw")
board_runner_args(openocd --gdb-init "mem 0x62FC0000 0x63010000 rw")
board_runner_args(openocd --gdb-init "mem 0x90000000 0x90020000 ro")
board_runner_args(openocd --gdb-init "mem 0xA8000000 0xA8800000 rw")
board_runner_args(openocd --gdb-init "mem 0xA0000000 0xA0400000 ro")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

board_runner_args(bflb_mcu_tool --chipname bl616)
include(${ZEPHYR_BASE}/boards/common/bflb_mcu_tool.board.cmake)

board_set_flasher(bflb_mcu_tool)
