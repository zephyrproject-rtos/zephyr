# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  soc.c
)

zephyr_library_sources_ifdef(CONFIG_ARM_MPU mpu_regions.c)
