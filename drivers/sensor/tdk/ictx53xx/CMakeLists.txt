# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ictx53xx.c)
zephyr_library_sources_ifdef(CONFIG_I2C ictx53xx_i2c.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API ictx53xx_async.c ictx53xx_decoder.c)

zephyr_library_include_directories(.)
