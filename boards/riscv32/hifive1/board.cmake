# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine sifive_e
  )

set(BOARD_DEBUG_RUNNER qemu)
set(BOARD_FLASH_RUNNER hifive1)
board_finalize_runner_args(hifive1)