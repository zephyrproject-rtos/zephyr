# Copyright (c) 2025 ENE Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_include_directories(.)
zephyr_include_directories(../common)
zephyr_library_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
