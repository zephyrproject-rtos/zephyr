# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_binary_suffix sparc)
set(QEMU_CPU_TYPE_${ARCH} leon3)

set(QEMU_FLAGS_${ARCH}
  -machine leon3_generic
  -m 1G
  -icount auto
  )
include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
