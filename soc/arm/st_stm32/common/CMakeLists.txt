# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_ARM_MPU arm_mpu_regions.c)
zephyr_sources(stm32cube_hal.c)

zephyr_linker_sources_ifdef(DT_CCM_BASE_ADDRESS SECTIONS ccm.ld)
