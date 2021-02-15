# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_binary_suffix riscv32)
set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine virt
  -bios none
  -m 256
  )
board_set_debugger_ifnset(qemu)
