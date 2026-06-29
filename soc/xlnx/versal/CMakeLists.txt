#
# Copyright (c) 2025 Space Cubics Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(CONFIG_SOC_VERSAL_RPU soc.c)
zephyr_sources_ifdef(CONFIG_SOC_VERSAL_RPU_OCM_BOOT rpu_ocm_boot.S)
zephyr_sources_ifdef(CONFIG_SOC_VERSAL_RPU_OCM_BOOT rpu_ocm_mpu_init.c)
zephyr_sources_ifdef(CONFIG_ARM_MMU arm_mmu_regions.c)

zephyr_sources_ifdef(
  CONFIG_ARM_MPU
  arm_mpu_regions.c
)

zephyr_include_directories(.)

if(CONFIG_SOC_VERSAL_RPU)
  # Use generic Cortex-A/R linker script
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()

if(CONFIG_SOC_VERSAL_RPU_OCM_BOOT)
  # Boot stub runs from rom_start @ 0x0; z_arm_relocate_vector_table() copies the
  # arch vector table over the stub once OCM is reachable.
  zephyr_linker_sources(
    ROM_START
    SORT_KEY 0x0boot
    rpu_ocm_boot.ld
  )
endif()

if(CONFIG_SOC_VERSAL_APU)
  # Use generic ARM64 linker script
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()
