# Copyright (c) 2025 Sentry Technologies ApS
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  iim4623x.c
  iim4623x_bus.c
)

zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API
  iim4623x_decoder.c
)
