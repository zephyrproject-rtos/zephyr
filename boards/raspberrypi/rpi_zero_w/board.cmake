# Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH arm)

set(QEMU_CPU_TYPE_${ARCH} arm1176)

set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine raspi0
  -nographic
  -icount shift=5,align=off,sleep=off
  )

board_set_debugger_ifnset(qemu)

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
