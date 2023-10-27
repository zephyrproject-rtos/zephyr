# SPDX-License-Identifier: Apache-2.0
zephyr_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
