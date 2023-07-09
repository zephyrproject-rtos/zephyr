#
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE_${ARCH} cortex-m0)
set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine microbit
  -nographic
  -vga none
  )
board_runner_args(qemu "--cpu=cortex-m0" "--machine=microbit")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
