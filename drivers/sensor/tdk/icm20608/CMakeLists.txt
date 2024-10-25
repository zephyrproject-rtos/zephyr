# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  icm20608.c
)

zephyr_library_sources_ifdef(CONFIG_ICM20608_TRIGGER icm20608_trigger.c)
