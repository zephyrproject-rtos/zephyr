# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_CPU_TYPE_${ARCH} cortex-m3)
set_property(TARGET   ${ZEPHYR_TARGET} PROPERTY   QEMU_FLAGS_${ARCH}
  -cpu cortex-m3
  -machine lm3s6965evb
  -nographic
  -vga none
  )

board_set_debugger_ifnset(qemu)
