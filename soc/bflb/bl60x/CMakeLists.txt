# Copyright (c) 2021-2025 ATL Electronics
# Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
