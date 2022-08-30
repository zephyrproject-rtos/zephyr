# Copyright (c) 2022 IoT.bzh
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ARM_MPU arm_mpu_regions.c)
