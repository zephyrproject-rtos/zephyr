# ST Microelectronics LIS2DE12 3-axis accelerometer sensor driver
#
# Copyright (c) 2024 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lis2de12.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DE12_TRIGGER    lis2de12_trigger.c)

zephyr_library_include_directories(../stmemsc)
