# ST Microelectronics LIS2DW12 3-axis accelerometer driver
#
# Copyright (c) 2019 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lis2dw12.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DW12_TRIGGER    lis2dw12_trigger.c)

zephyr_library_include_directories(../stmemsc)
