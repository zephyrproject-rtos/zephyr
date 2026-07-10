# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

include(${ZEPHYR_BASE}/boards/common/qemu_riscv.board.cmake)

qemu_riscv_cpu_from_dt(qemu_riscv_cpu)
qemu_riscv_binary_suffix(QEMU_binary_suffix)

set(QEMU_CPU_TYPE_${ARCH} "${qemu_riscv_cpu}")

set(QEMU_FLAGS_${ARCH}
  -machine virt
  -bios none
  -m 256
  -cpu ${qemu_riscv_cpu}
  )

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
