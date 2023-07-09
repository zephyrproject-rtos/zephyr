# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix riscv64)
set(QEMU_CPU_TYPE_${ARCH} riscv64)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine virt
  -bios none
  -m 256
  )
board_runner_args(qemu "--commander=qemu-system-riscv64" "--machine=virt"
              "--qemu_option=${CMAKE_CURRENT_LIST_DIR}/qemu_riscv64_option.yml")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
