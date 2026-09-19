# Copyright (c) 2026 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(admt4000.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API admt4000_decoder.c admt4000_async.c)
