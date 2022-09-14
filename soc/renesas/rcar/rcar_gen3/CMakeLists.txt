# Copyright (c) 2021 IoT.bzh
# Copyright (c) 2023 EPAM Systems
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_SERIES_RCAR_GEN3_R7)
  zephyr_sources(r7/soc.c)
  zephyr_include_directories(r7)
  zephyr_library_sources_ifdef(CONFIG_SOC_R8A77951_R7 pfc_r8a77951.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_SERIES_RCAR_GEN3_A57)
  zephyr_include_directories(a57)
  zephyr_library_sources_ifdef(CONFIG_SOC_R8A77951_A57 pfc_r8a77951.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_R8A77961        pfc_r8a77961.c)
  zephyr_library_sources_ifdef(CONFIG_ARM_MMU             mmu_regions.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
