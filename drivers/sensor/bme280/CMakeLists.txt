# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BME280 bme280.c bme280_spi.c bme280_i2c.c)
