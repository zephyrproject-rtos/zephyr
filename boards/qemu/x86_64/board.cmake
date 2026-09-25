# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Intel Corp.

set(QEMU_CPU_TYPE qemu64,+x2apic)

if("${CONFIG_MP_MAX_NUM_CPUS}" STREQUAL "1")
  # icount works with 1 CPU so we can enable it here.
  # FIXME: once this works across configs, remove this line and set
  # CONFIG_QEMU_ICOUNT_SHIFT in defconfig instead.
  list(APPEND QEMU_EXTRA_FLAGS -icount shift=5,align=off,sleep=off -rtc clock=vm)
endif()

include(${ZEPHYR_BASE}/boards/common/qemu_x86.board.cmake)
