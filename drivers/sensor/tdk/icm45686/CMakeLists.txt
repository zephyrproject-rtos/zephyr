# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  icm45686.c
)

zephyr_library_sources_ifdef(CONFIG_SPI icm45686_spi.c)
zephyr_library_sources_ifdef(CONFIG_I2C icm45686_i2c.c)
zephyr_library_sources_ifdef(CONFIG_USE_EMD_ICM45686 icm45686_e.c)
zephyr_library_sources_ifdef(CONFIG_ICM45686_TRIGGER icm45686_trigger.c)
