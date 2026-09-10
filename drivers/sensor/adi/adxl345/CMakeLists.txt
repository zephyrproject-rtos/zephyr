#
#  Copyright (c) 2020 Antmicro <www.antmicro.com>
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(adxl345.c)
zephyr_library_sources_ifdef(CONFIG_ADXL345_TRIGGER adxl345_trigger.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API adxl345_rtio.c adxl345_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ADXL345_STREAM adxl345_stream.c adxl345_decoder.c)
