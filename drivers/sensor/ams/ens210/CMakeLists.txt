# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ens210.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API ens210_rtio.c)
