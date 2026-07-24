#
# Copyright (c) 2021 Bosch Sensortec GmbH
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(bmi260.c)
zephyr_library_sources_ifdef(CONFIG_BMI260_BUS_I2C bmi260_i2c.c)
zephyr_library_sources_ifdef(CONFIG_BMI260_BUS_SPI bmi260_spi.c)
zephyr_library_sources_ifdef(CONFIG_BMI260_TRIGGER bmi260_trigger.c)
