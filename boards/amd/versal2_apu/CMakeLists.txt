#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025-2026 Advanced Micro Devices, Inc.
#

set(QEMU_DT_NAME "board-versal2-psxc-virt")
set(QEMU_CONSOLE_SERIAL_REG 0xf1930000)
include(${ZEPHYR_BASE}/boards/amd/common/qemu_hw_dtb.cmake)
