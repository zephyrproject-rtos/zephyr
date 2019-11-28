#
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} cortex-m0)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine microbit
  -nographic
  -vga none
  )

board_set_debugger_ifnset(qemu)
