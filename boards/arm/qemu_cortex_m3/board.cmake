# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE_${ARCH} cortex-m3)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine lm3s6965evb
  -nographic
  -vga none
  )

board_runner_args(qemu "--cpu=cortex-m3" "--machine=lm3s6965evb")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
