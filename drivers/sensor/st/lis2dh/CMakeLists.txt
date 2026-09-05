# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(lis2dh.c)
zephyr_library_sources(lis2dh_i2c.c)
zephyr_library_sources(lis2dh_spi.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DH_TRIGGER lis2dh_trigger.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DH_FIFO lis2dh_fifo.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API
  lis2dh_rtio.c
  lis2dh_decoder.c
)
zephyr_library_sources_ifdef(CONFIG_LIS2DH_STREAM lis2dh_stream.c)
