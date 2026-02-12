# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tad214x_drv.c)
zephyr_library_sources_ifdef(CONFIG_SPI tad214x_spi.c)
zephyr_library_sources_ifdef(CONFIG_I2C tad214x_i2c.c)

zephyr_library_sources_ifdef(CONFIG_TAD2144_TRIGGER tad214x_trigger.c)

