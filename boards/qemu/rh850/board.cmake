# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_BINARY_SUFFIX rh850)

if(CONFIG_BOARD_QEMU_RH850_U2A)
  set(QEMU_MACHINE rh850-u2a)
else()
  set(QEMU_MACHINE rh850-u2c)
endif()

set(QEMU_BOARD_FLAGS
  -machine ${QEMU_MACHINE}
  -nographic
)

board_set_debugger_ifnset(qemu)
