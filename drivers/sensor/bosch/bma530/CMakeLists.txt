# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bma530.c)
zephyr_library_sources(bma530_i2c.c)
zephyr_library_sources(bma530_spi.c)
