# Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_BINARY_SUFFIX riscv64)
set(QEMU_CPU_TYPE thead-c908)

set(QEMU_BOARD_FLAGS
  -machine k230
)

# K230 runs Zephyr directly in M-mode via QEMU's firmware boot path (-bios).
set(QEMU_KERNEL_OPTION "-bios;\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
