# Copyright (c) 2025, FocalTech Systems CO.,Ltd
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
    soc.c
    ft9001_xip.c
)

zephyr_include_directories(.)

zephyr_code_relocate(FILES ft9001_xip.c LOCATION RAM)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
