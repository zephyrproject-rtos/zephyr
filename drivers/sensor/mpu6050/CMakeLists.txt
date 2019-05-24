# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MPU6050 mpu6050.c)
zephyr_library_sources_ifdef(CONFIG_MPU6050_TRIGGER mpu6050_trigger.c)
