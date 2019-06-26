# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_CPU_TYPE_${ARCH} cortex-m33)
set_property(TARGET   ${ZEPHYR_TARGET} PROPERTY   QEMU_FLAGS_${ARCH}
  -cpu cortex-m33
  -machine mps2-an521
  -nographic
  -m 16
  -vga none
  )

board_set_debugger_ifnset(qemu)
