# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_binary_suffix riscv64)
set(QEMU_CPU_TYPE_${ARCH} riscv64)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine sifive_e
  )

if(CONFIG_QEMU_ICOUNT)
  list(APPEND QEMU_EXTRA_FLAGS -icount shift=6,align=off,sleep=off -rtc clock=vm)
endif()
board_set_debugger_ifnset(qemu)
