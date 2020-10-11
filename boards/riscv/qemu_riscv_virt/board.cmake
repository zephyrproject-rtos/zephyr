# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

if(CONFIG_64BIT)
  set(QEMU_binary_suffix riscv64)
  set(QEMU_CPU_TYPE_${ARCH} riscv64)
else()
  set(QEMU_binary_suffix riscv32)
  set(QEMU_CPU_TYPE_${ARCH} riscv32)
endif()

set(QEMU_FLAGS_${ARCH}
  -m 4M
  -nographic
  -machine virt
  )

board_set_debugger_ifnset(qemu)
