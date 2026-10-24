# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_CPU_TYPE cortex-m33)
set(QEMU_BOARD_FLAGS
  -cpu ${QEMU_CPU_TYPE}
  -machine mps2-an521
  -m 16
  -vga none
  )

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
