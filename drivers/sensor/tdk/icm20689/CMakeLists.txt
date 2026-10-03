# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_ICM20689
  icm20689.c
  icm20689_spi.c
)

zephyr_library_sources_ifdef(CONFIG_ICM20689_TRIGGER icm20689_trigger.c)
