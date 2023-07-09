# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE_${ARCH} 24Kc)

set(QEMU_FLAGS_${ARCH}
  -machine malta
  -nographic
  -serial null
  -serial null
  )

board_runner_args(qemu "--commander=qemu-system-24Kc" "--machine=malta"
                "--qemu_option=${CMAKE_CURRENT_LIST_DIR}/qemu_malta_option.yml")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
