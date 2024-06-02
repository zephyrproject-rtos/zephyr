#
#  Copyright (c) 2024 Aaron Chan
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(adxl375.c)
zephyr_library_sources(adxl375_spi.c)
zephyr_library_sources(adxl375_i2c.c)
