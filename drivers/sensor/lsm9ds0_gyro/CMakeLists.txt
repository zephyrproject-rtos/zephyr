# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LSM9DS0_GYRO lsm9ds0_gyro.c)
zephyr_library_sources_ifdef(CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY lsm9ds0_gyro_trigger.c)
