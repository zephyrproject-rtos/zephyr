# Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix riscv32)
set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine virt
  -bios none
  -m 256
  )

board_runner_args(qemu "--commander=qemu-system-riscv32" "--machine=virt"
              "--qemu_option=${CMAKE_CURRENT_LIST_DIR}/qemu_riscv32e_option.yml")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
