#
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE cortex-m0)
set(QEMU_BOARD_FLAGS
  -cpu ${QEMU_CPU_TYPE}
  -machine microbit
  )
include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
