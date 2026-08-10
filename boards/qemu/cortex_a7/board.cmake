# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE cortex-a7)

if(CONFIG_ARMV7_A_NS)
  set(QEMU_MACH virt,gic-version=3)
else()
  set(QEMU_MACH virt,secure=on,gic-version=3)
endif()

set(QEMU_BOARD_FLAGS
  -cpu ${QEMU_CPU_TYPE}
  -machine ${QEMU_MACH}
  )

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
