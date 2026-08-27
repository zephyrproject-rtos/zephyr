# Copyright (c) 2025 Croxel Inc.
# Copyright (c) 2025 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_include_directories(.)
zephyr_library_include_directories(${ZEPHYR_HAL_TDK_MODULE_DIR}/icm566xx)

zephyr_library_sources(
  icm566xx.c
  icm566xx_bus.c
)

zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API icm566xx_decoder.c)
zephyr_library_sources_ifdef(CONFIG_ICM566XX_STREAM icm566xx_stream.c)
zephyr_library_sources_ifdef(CONFIG_ICM566XX_TRIGGER icm566xx_trigger.c)
