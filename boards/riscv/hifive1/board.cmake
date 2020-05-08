# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine sifive_e
  )

if(CONFIG_QEMU_ICOUNT)
  list(APPEND QEMU_EXTRA_FLAGS -icount shift=6,align=off,sleep=off -rtc clock=vm)
endif()

board_set_debugger_ifnset(qemu)
board_set_flasher_ifnset(hifive1)
board_finalize_runner_args(hifive1)
