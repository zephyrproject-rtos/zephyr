# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    vl53l5cx.c
    vl53l5cx_platform.c
)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API vl53l5cx_rtio.c vl53l5cx_decoder.c)
