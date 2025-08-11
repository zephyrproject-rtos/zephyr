#
# Copyright (c) 2022 Badgerd Technologies B.V and its affiliates
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_include_directories(.)
zephyr_library_sources(
	bmp581.c
	bmp581_bus.c
)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API
	bmp581_decoder.c
)

zephyr_library_sources_ifdef(CONFIG_BMP581_STREAM
	bmp581_stream.c
)
