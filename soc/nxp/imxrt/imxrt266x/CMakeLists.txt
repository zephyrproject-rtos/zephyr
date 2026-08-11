#
# SPDX-FileCopyrightText: Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

# This core's image: its bring-up code and its memory layout.
zephyr_sources_ifdef(CONFIG_SOC_SERIES_IMXRT266X_CM85
  soc.c
  soc_clock.c
  trdc_setup.c
)

# MPU regions come from the generic arch/arm/core/mpu/arm_mpu_regions.c, which
# builds them from the chosen nodes: FLASH_0 = zephyr,flash (the XSPI0 NOR
# window) and SRAM_0 = zephyr,sram (the XSPI1 PSRAM). Everything else, the
# peripheral windows included, falls through to the ARMv8-M default memory map,
# which the MPU is enabled with (PRIVDEFENA). The devicetree memory nodes
# deliberately do NOT carry zephyr,memory-attr, so the DT path does not ALSO
# program those regions -- on ARMv8-M (M85) two overlapping enabled MPU regions
# at the same address fault the moment the MPU is enabled.

zephyr_include_directories(.)

if(CONFIG_SOC_SERIES_IMXRT266X_CM85)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
endif()
