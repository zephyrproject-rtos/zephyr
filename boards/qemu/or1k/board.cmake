# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE or1k)

set(QEMU_BOARD_FLAGS
  -machine virt
)

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
