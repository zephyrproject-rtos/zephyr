# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  icm40627.c
  icm40627_i2c.c
)

zephyr_library_sources_ifdef(CONFIG_ICM40627_TRIGGER icm40627_trigger.c)
