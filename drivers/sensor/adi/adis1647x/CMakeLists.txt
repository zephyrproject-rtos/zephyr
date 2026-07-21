# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(adis1647x.c)
zephyr_library_sources(adis1647x_decoder.c)
zephyr_library_sources(adis1647x_spi.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API adis1647x_rtio.c)
zephyr_library_sources_ifdef(CONFIG_ADIS1647X_TRIGGER adis1647x_trigger.c)
zephyr_library_sources_ifdef(CONFIG_ADIS1647X_STREAM adis1647x_stream.c)
