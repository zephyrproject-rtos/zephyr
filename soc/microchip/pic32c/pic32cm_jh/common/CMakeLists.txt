# Copyright (c) 2025-2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_include_directories(.)

zephyr_library_sources(supc_mchp.c)
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
