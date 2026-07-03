# Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH arm)

set(QEMU_CPU_TYPE_${ARCH} arm1176)

# raspi0 wires PL011 to serial0 and the AUX mini-UART to serial1. The
# Zephyr console is the mini-UART, so claim serial0 (PL011) with null here;
# qemu.board.cmake appends "-serial chardev:con" which then lands on serial1
# (the mini-UART) and is muxed to stdio.
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine raspi0
  -serial null
  -icount shift=5,align=off,sleep=off
  )

board_set_debugger_ifnset(qemu)

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
