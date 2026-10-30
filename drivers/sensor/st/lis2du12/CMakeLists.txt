# Copyright (c) 2023 STMicroelectronics
# Copyright (c) 2026 Dotcom IoT LLP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(lis2du12.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API lis2du12_decoder.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DU12_TRIGGER lis2du12_trigger.c)

zephyr_library_include_directories(../stmemsc)
