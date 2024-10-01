#
# Copyright (c) 2021 Bosch Sensortec GmbH
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(bmi270.c)
zephyr_library_sources_ifdef(CONFIG_BMI270_BUS_I2C bmi270_i2c.c)
zephyr_library_sources_ifdef(CONFIG_BMI270_BUS_SPI bmi270_spi.c)
zephyr_library_sources_ifdef(CONFIG_BMI270_TRIGGER bmi270_trigger.c)
