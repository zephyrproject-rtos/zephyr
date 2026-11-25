#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 Advanced Micro Devices, Inc.
#

# The ZynqMP Cadence UART carries no chardev property; QEMU binds it to the
# -serial backend by index, so the hw-dtb is used verbatim (no console patch).
set(QEMU_DT_NAME "board-zynqmp-zcu102")
include(${ZEPHYR_BASE}/boards/amd/common/qemu_hw_dtb.cmake)
