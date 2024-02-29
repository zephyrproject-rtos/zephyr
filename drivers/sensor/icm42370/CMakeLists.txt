# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  icm42370.c
)

zephyr_library_sources_ifdef(CONFIG_SPI icm42370_spi.c)
zephyr_library_sources_ifdef(CONFIG_I2C icm42370_i2c.c)
zephyr_library_sources_ifdef(CONFIG_ICM42370_TRIGGER icm42370_trigger.c)
