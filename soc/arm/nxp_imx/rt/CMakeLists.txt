#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_ARM_MPU arm_mpu_regions.c)
