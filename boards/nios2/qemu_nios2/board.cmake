# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE_${ARCH} nios2)

set(QEMU_FLAGS_${ARCH}
  -machine altera_10m50_zephyr
  -nographic
  )

board_runner_args(qemu "--commander=qemu-system-nios2" "--machine=altera_10m50_zephyr")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
