# ST Microelectronics LIS2DUXS12 3-axis accelerometer sensor driver
#
# Copyright (c) 2023 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lis2duxs12.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DUXS12_TRIGGER    lis2duxs12_trigger.c)

zephyr_library_include_directories(../stmemsc)
