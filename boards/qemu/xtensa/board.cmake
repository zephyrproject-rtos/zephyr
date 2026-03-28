# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

if(CONFIG_BOARD_QEMU_XTENSA)
  set(QEMU_CPU_TYPE_${ARCH} ${CONFIG_SOC})

  set(QEMU_FLAGS_${ARCH}
    -machine sim -semihosting -cpu ${CONFIG_SOC}
  )
endif()

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
