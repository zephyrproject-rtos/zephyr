# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  power.c
  )
zephyr_sources_ifdef(CONFIG_ARM_MPU arm_mpu_regions.c)
