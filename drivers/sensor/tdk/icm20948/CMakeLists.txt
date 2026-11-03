# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(icm20948.c)
zephyr_library_sources_ifdef(CONFIG_I2C icm20948_i2c.c)
zephyr_library_sources_ifdef(CONFIG_SPI icm20948_spi.c)
zephyr_library_sources_ifdef(CONFIG_ICM20948_MAGN_EN icm20948_ak09916.c)
