# Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE arm1176)

# raspi0 wires the PL011 to serial0 and the AUX mini-UART to serial1. The
# console is the mini-UART, so serial0 is claimed with null here and the
# console chardev lands on serial1.
set(QEMU_BOARD_FLAGS
  -cpu ${QEMU_CPU_TYPE}
  -machine raspi0
  -serial null
  )

board_set_debugger_ifnset(qemu)

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
