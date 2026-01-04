# Copyright (c) 2025 Synopsys, Inc.
# SPDX-License-Identifier: Apache-2.0

# Use common SoC implementation
zephyr_include_directories(${ZEPHYR_BASE}/soc/qemu/virt_riscv/common)
zephyr_library_sources(${ZEPHYR_BASE}/soc/qemu/virt_riscv/common/soc.c)
