# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(mpu9250.c)
zephyr_library_sources_ifdef(CONFIG_MPU9250_TRIGGER mpu9250_trigger.c)
zephyr_library_sources_ifdef(CONFIG_MPU9250_MAGN_EN ak8963.c)
