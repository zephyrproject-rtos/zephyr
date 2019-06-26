# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_binary_suffix    riscv64)
set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_CPU_TYPE_${ARCH} riscv64)
set_property(TARGET ${ZEPHYR_TARGET} PROPERTY QEMU_FLAGS_${ARCH}
  -nographic
  -machine sifive_e
  )

board_set_debugger_ifnset(qemu)
