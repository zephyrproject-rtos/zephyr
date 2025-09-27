# Copyright (c) 2018 Bosch Sensortec GmbH
# Copyright (c) 2022, Leonard Pollak
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(bme680.c bme680_bus.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API bme680_decoder.c bme680_async.c)
