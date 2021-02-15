# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_binary_suffix sparc)
set(QEMU_CPU_TYPE_${ARCH} leon3)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine leon3_generic
  -icount auto
  )
board_set_debugger_ifnset(qemu)
