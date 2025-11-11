#
#  Copyright (c) 2018 Analog Devices Inc.
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(adxl372.c)
zephyr_library_sources(adxl372_spi.c)
zephyr_library_sources(adxl372_i2c.c)
zephyr_library_sources_ifdef(CONFIG_ADXL372_TRIGGER adxl372_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API adxl372_rtio.c adxl372_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ADXL372_STREAM adxl372_stream.c adxl372_decoder.c)
