# Copyright 2023-2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_ARM_MPU nxp_mpu_regions.c)

zephyr_sources_ifdef(CONFIG_NXP_S32_FLASH_CONFIG flash_configuration.c)
zephyr_linker_sources_ifdef(CONFIG_NXP_S32_FLASH_CONFIG ROM_START SORT_KEY 0x1 flash_config.ld)
