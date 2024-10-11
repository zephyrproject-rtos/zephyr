# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(adxl362.c)
zephyr_library_sources_ifdef(CONFIG_ADXL362_TRIGGER adxl362_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API adxl362_rtio.c adxl362_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ADXL362_STREAM adxl362_stream.c adxl362_decoder.c)
