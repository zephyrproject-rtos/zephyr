# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_BINARY_SUFFIX tricore)
set(QEMU_CPU_TYPE tc37x)

if(CONFIG_BOARD_QEMU_TC3X)
  set(QEMU_BOARD_FLAGS
    -machine KIT_AURIX_TC397B_TRB
    -nographic
  )
elseif(CONFIG_BOARD_QEMU_TC4X)
  set(QEMU_BOARD_FLAGS
    -machine KIT_A3G_TC4D7_LITE
    -nographic
  )
endif()

board_set_debugger_ifnset(qemu)
