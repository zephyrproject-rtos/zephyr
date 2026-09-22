# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(mmc56x3.c mmc56x3_i2c.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API mmc56x3_decoder.c mmc56x3_async.c)
