# Copyright (c) 2025 Croxel
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(als31300.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API als31300_async.c als31300_decoder.c)
