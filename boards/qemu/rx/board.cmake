# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_MACH gdbsim-r5f562n8)

set(QEMU_FLAGS_${ARCH}
  -machine ${QEMU_MACH}
  )

if(CONFIG_XIP)
  set(QEMU_KERNEL_OPTION
  -bios ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  )
endif()

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
