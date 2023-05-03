# Copyright (c) 2023 EPAM Systems
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_SOC_R8A779F  pfc_r8a779f.c)
zephyr_library_sources_ifdef(CONFIG_ARM_MMU      mmu_regions.c)
