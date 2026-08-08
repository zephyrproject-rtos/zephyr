# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH aarch64)

set(QEMU_CPU_TYPE_${ARCH} cortex-a7)

if(CONFIG_ARMV7_A_NS)
  set(QEMU_MACH virt,gic-version=2)
else()
  set(QEMU_MACH virt,secure=on,gic-version=2)
endif()

set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -machine ${QEMU_MACH}
  )

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
