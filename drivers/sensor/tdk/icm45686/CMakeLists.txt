# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(.)
zephyr_library_sources(
	icm45686.c
)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API
	icm45686_decoder.c
)
zephyr_library_sources_ifdef(CONFIG_ICM45686_TRIGGER
	icm45686_trigger.c
)
zephyr_library_sources_ifdef(CONFIG_ICM45686_STREAM
	icm45686_stream.c
)
