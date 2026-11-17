#
#  Copyright (c) 2026 Deere & Company
#
#  SPDX-License-Identifier: Apache-2.0
#

zephyr_library_named(adxl382)
zephyr_library_sources(adxl382.c adxl382_spi.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API adxl382_rtio.c adxl382_decoder.c)
