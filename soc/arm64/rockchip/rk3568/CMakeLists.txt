# SPDX-License-Identifier: Apache-2.0
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)

zephyr_library_sources_ifdef(CONFIG_SOC_RK3568 plat_core.c)
zephyr_library_sources_ifdef(CONFIG_SOC_RK3568 soc.c)