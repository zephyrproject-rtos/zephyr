# Copyright (c) 2026 Shreyas Acharya
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bmi260_bus.c)
zephyr_library_sources_ifdef(CONFIG_BMI260_BUS_I2C bmi260_i2c.c)
zephyr_library_sources_ifdef(CONFIG_BMI260_BUS_SPI bmi260_spi.c)
