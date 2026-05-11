# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics

zephyr_library()

zephyr_library_include_directories(
     ${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l5cx
     ${ZEPHYR_HAL_ST_MODULE_DIR}/sensor/vl53l5cx/zephyr
)

zephyr_library_sources(
    vl53l5cx.c
    vl53l5cx_platform.c
)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API vl53l5cx_rtio.c vl53l5cx_decoder.c)
