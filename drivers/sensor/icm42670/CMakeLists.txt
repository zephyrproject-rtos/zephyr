# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  icm42670.c
  icm42670_spi.c
)

zephyr_library_sources_ifdef(CONFIG_ICM42670_TRIGGER icm42670_trigger.c)
