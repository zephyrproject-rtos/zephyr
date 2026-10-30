# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE or1k)

if(CONFIG_BOARD_QEMU_OR1K_QEMU_OR1K_SIM)
  set(QEMU_MACH or1k-sim)
else()
  set(QEMU_MACH virt)
endif()

set(QEMU_BOARD_FLAGS
  -machine ${QEMU_MACH}
)

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
