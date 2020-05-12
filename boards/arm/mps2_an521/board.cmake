# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)
set(TFM_TARGET_PLATFORM "AN521")

set(QEMU_CPU_TYPE_${ARCH} cortex-m33)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine mps2-an521
  -nographic
  -m 16
  -vga none
  )

if(CONFIG_QEMU_ICOUNT)
  list(APPEND QEMU_EXTRA_FLAGS -icount shift=7,align=off,sleep=off -rtc clock=vm)
endif()
board_set_debugger_ifnset(qemu)
