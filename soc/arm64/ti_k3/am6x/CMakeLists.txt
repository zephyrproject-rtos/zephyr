# Copyright (c) 2023 Enphase Energy
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)
