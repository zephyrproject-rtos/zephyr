#
#  Copyright (c) 2023 Analog Devices Inc.
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(adxl367.c)
zephyr_library_sources(adxl367_spi.c)
zephyr_library_sources(adxl367_i2c.c)
