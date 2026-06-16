# Copyright (c) 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH aarch64)

set(QEMU_CPU_TYPE_${ARCH} cortex-a72)

set(QEMU_FLAGS_${ARCH}
  -machine raspi4b
)

set(QEMU_KERNEL_OPTION
  -kernel ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
)

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
