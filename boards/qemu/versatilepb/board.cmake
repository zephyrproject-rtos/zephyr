# Copyright (c) 2026 Takayuki Goto
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_BINARY_SUFFIX arm)
set(QEMU_CPU_TYPE arm926)

set(QEMU_BOARD_FLAGS
  -machine versatilepb
  -m 64
  -nographic
  )

board_set_debugger_ifnset(qemu)
