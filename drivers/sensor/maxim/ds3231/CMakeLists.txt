# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ds3231.c)
#zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API bme280_decoder.c bme280_async.c)
