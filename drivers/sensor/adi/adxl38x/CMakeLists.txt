# Copyright (c) 2026 Analog Devices Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(adxl38x.c)
zephyr_library_sources_ifdef(CONFIG_ADXL38X_TRIGGER adxl38x_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API adxl38x_rtio.c adxl38x_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ADXL38X_STREAM adxl38x_stream.c adxl38x_decoder.c)
