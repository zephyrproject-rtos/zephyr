# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix sparc)
set(QEMU_CPU_TYPE_${ARCH} leon3)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine leon3_generic
  -icount auto
  )
board_runner_args(qemu "--commander=qemu-system-leon3" "--machine=leon3_generic"
  "--qemu_option=${CMAKE_CURRENT_LIST_DIR}/qemu_leon3_option.yml")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
