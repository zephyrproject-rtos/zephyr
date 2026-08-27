# Texas Instruments AM13E MCU Series
#
# Copyright (c) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_ARM_MPU mpu_regions.c)
