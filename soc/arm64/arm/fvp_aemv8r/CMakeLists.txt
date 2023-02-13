# Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0
zephyr_library_sources(
  soc.c
)

zephyr_library_sources_ifdef(CONFIG_ARM_MPU arm_mpu_regions.c)
