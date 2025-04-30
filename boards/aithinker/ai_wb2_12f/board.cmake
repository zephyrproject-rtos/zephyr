# Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-pre-init "source [find bl60x.cfg]")

board_runner_args(openocd --use-elf --no-load --no-init)
board_runner_args(openocd --gdb-init "set mem inaccessible-by-default off")
board_runner_args(openocd --gdb-init "set architecture riscv:rv32")
board_runner_args(openocd --gdb-init "set remotetimeout 250")
board_runner_args(openocd --gdb-init "set print asm-demangle on")
board_runner_args(openocd --gdb-init "set backtrace limit 32")
board_runner_args(openocd --gdb-init "mem 0x22008000 0x22014000 rw")
board_runner_args(openocd --gdb-init "mem 0x42008000 0x42014000 rw")
board_runner_args(openocd --gdb-init "mem 0x22014000 0x22020000 rw")
board_runner_args(openocd --gdb-init "mem 0x42014000 0x42020000 rw")
board_runner_args(openocd --gdb-init "mem 0x22020000 0x2203C000 rw")
board_runner_args(openocd --gdb-init "mem 0x42020000 0x4203C000 rw")
board_runner_args(openocd --gdb-init "mem 0x23000000 0x23400000 ro")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
