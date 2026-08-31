# Copyright (c) 2026 MediaTek Inc.
# SPDX-License-Identifier: Apache-2.0

# Only reached from the family CMakeLists when the target is not an audio DSP,
# so this covers the Arm clusters of this die, all of which are arm64.
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")

if(CONFIG_SOC_MT8188_A55)
  zephyr_include_directories(a55)

  zephyr_sources_ifdef(CONFIG_ARM_MMU a55/mmu_regions.c)
endif()
