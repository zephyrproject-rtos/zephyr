# Copyright (c) 2016 Zephyr Contributors
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE_${ARCH} cortex-m3)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine lm3s6965evb
  )
include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
