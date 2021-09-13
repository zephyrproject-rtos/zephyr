# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS renode qemu)
set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/hifive1.resc)

set(QEMU_binary_suffix riscv32)
set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine sifive_e
  )

board_set_debugger_ifnset(qemu)
board_set_flasher_ifnset(hifive1)
board_finalize_runner_args(hifive1)
