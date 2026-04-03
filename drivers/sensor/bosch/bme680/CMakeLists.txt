# Copyright (c) 2018 Bosch Sensortec GmbH
# Copyright (c) 2022, Leonard Pollak
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(bme680.c bme680_i2c.c bme680_spi.c)
