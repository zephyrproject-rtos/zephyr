# Copyright 2025 sevenlab engineering GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(.)
zephyr_library_sources(
    sen6x.c
)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API
    sen6x_decoder.c
)
zephyr_library_sources_ifdef(CONFIG_SEN6X_SHELL
    shell.c
)
