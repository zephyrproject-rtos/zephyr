# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ARC_CORE_MPU arc_core_mpu.c)
zephyr_library_sources_ifdef(CONFIG_ARC_MPU arc_mpu.c)
