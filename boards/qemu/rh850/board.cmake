# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_BINARY_SUFFIX rh850)

set(QEMU_BOARD_FLAGS
  -machine rh850-u2c
  -nographic
)

board_set_debugger_ifnset(qemu)
