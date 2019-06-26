# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_CPU_TYPE_${ARCH} riscv32)
set_property(TARGET   ${ZEPHYR_TARGET} PROPERTY   QEMU_FLAGS_${ARCH}
  -nographic
  -machine sifive_e
  )

board_set_debugger_ifnset(qemu)
board_set_flasher_ifnset(hifive1)
board_finalize_runner_args(hifive1)
