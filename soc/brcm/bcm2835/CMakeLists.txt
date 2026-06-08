# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(pinctrl.c)
zephyr_sources_ifdef(CONFIG_ARM_AARCH32_MMU mmu_regions.c)
zephyr_sources_ifdef(CONFIG_EARLY_CONSOLE early_console.c)
zephyr_cc_option_ifdef(CONFIG_ARMV6_ARM1176 -mno-unaligned-access)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld
  CACHE INTERNAL "")
