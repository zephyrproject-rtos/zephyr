# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(adxl355.c)
zephyr_library_sources_ifdef(CONFIG_ADXL355_TRIGGER adxl355_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API adxl355_rtio.c adxl355_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ADXL355_STREAM adxl355_stream.c adxl355_decoder.c)
