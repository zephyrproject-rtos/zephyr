# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# Copyright (c) 2026 Swarovski Optik AG & Co KG.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(.)
zephyr_library_sources(
        icm45605.c
        icm45605_bus.c
)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API
        icm45605_decoder.c
)
zephyr_library_sources_ifdef(CONFIG_ICM45605_TRIGGER
        icm45605_trigger.c
)
zephyr_library_sources_ifdef(CONFIG_ICM45605_STREAM
        icm45605_stream.c
)
