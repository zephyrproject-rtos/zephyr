# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

if(CONFIG_CPU_M68000)
  set(QEMU_CPU_TYPE m68000)
elseif(CONFIG_CPU_M68010)
  set(QEMU_CPU_TYPE m68010)
endif()

dt_chosen(QEMU_RAM_NODE PROPERTY "zephyr,sram")
dt_reg_size(QEMU_RAM_SIZE PATH "${QEMU_RAM_NODE}")
math(EXPR QEMU_RAM_SIZE_MB "${QEMU_RAM_SIZE} / 1048576")

set(QEMU_BOARD_FLAGS
  -machine virt
  -cpu ${QEMU_CPU_TYPE}
  -m ${QEMU_RAM_SIZE_MB}M
  -rtc clock=vm
)

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
