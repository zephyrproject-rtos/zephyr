# SPDX-License-Identifier: Apache-2.0
zephyr_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)
zephyr_include_directories_ifdef(CONFIG_BCM2711_VC_IF ../common)
zephyr_sources_ifdef(CONFIG_BCM2711_VC_IF
    ../common/videocore_if.c
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
