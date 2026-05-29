# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE_${ARCH} 24Kc)

set(QEMU_FLAGS_${ARCH}
  -machine malta
  -serial null
  -serial null
  )
include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
