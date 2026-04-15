# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix tricore)
set(QEMU_CPU_TYPE_${ARCH} tc37x)

if(CONFIG_BOARD_QEMU_TC3X)
  set(QEMU_FLAGS_${ARCH}
    -machine KIT_AURIX_TC397B_TRB
    -nographic
  )
elseif(CONFIG_BOARD_QEMU_TC4X)
  set(QEMU_FLAGS_${ARCH}
    -machine KIT_A3G_TC4D7_LITE
    -nographic
  )
endif()

board_set_debugger_ifnset(qemu)
