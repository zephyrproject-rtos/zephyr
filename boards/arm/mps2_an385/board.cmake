# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} cortex-m3)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine mps2-an385
  -nographic
  -vga none
  -icount auto
  )

board_set_debugger_ifnset(qemu)
