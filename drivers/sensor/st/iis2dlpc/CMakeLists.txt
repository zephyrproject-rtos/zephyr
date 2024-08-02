# ST Microelectronics IIS2DLPC 3-axis accelerometer driver
#
# Copyright (c) 2020 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(iis2dlpc.c)
zephyr_library_sources_ifdef(CONFIG_IIS2DLPC_TRIGGER    iis2dlpc_trigger.c)

zephyr_library_include_directories(../stmemsc)
