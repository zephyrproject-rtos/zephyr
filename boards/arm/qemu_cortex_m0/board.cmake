#
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)

set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_CPU_TYPE_${ARCH} cortex-m0)
set_property(TARGET   ${ZEPHYR_TARGET} PROPERTY   QEMU_FLAGS_${ARCH}
  -cpu cortex-m0
  -machine microbit
  -nographic
  -vga none
  )

board_set_debugger_ifnset(qemu)
