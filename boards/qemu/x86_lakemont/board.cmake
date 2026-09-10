# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021 Intel Corporation.

set(QEMU_CPU_TYPE qemu32,+nx,+pae)

include(${ZEPHYR_BASE}/boards/common/qemu_x86.board.cmake)
