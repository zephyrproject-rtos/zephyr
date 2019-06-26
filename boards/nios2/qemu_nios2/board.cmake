# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_CPU_TYPE_${ARCH} nios2)
set_property(TARGET   ${ZEPHYR_TARGET} PROPERTY   QEMU_FLAGS_${ARCH}
  -machine altera_10m50_zephyr
  -nographic
  )

board_set_debugger_ifnset(qemu)
