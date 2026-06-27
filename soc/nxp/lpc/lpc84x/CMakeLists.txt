# Copyright (c) 2026 Aerlync Labs Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

# Linker script
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
