# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(                             arm_core_mpu.c)
zephyr_library_sources_ifdef(CONFIG_CPU_HAS_ARM_MPU arm_mpu.c)
zephyr_library_sources_ifdef(CONFIG_CPU_HAS_NXP_MPU nxp_mpu.c)
