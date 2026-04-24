# Copyright (c) 2024 Infineon Technologies AG
# SPDX-License-Identifier: Apache-2.0
zephyr_sources(soc.c cpu_idle.c cache.c)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/tricore/linker.ld CACHE INTERNAL "")
