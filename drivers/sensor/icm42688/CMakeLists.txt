# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  icm42688.c
  icm42688_spi.c
)

zephyr_library_sources_ifdef(CONFIG_ICM42688_TRIGGER icm42688_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_VERSION_1 icm42688_v1.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_VERSION_2 icm42688_v2.c)
