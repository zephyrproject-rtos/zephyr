# Copyright (c) 2025 Alif Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_ARM_MPU mpu_regions.c)
