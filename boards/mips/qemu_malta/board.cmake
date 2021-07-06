# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} 24Kc)

set(QEMU_FLAGS_${ARCH}
  -machine malta
  -nographic
  -serial null
  -serial null
  )
board_set_debugger_ifnset(qemu)
