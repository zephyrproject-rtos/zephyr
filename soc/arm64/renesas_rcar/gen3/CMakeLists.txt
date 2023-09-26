# Copyright (c) 2023 EPAM Systems
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_SOC_ARM64_R8A77951  pfc_r8a77951.c)
zephyr_library_sources_ifdef(CONFIG_SOC_R8A77961        pfc_r8a77961.c)
zephyr_library_sources_ifdef(CONFIG_ARM_MMU             mmu_regions.c)
