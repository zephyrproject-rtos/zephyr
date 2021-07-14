# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(
	soc_a53.c
)

zephyr_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)
