# Copyright (c) 2024 Abe Kohandel <abe.kohandel@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE} .)

zephyr_sources_ifdef(CONFIG_ARM_AARCH32_MMU mmu_regions.c)
zephyr_sources(irq.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
