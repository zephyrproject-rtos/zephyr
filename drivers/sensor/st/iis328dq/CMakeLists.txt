# ST Microelectronics IIS328DQ 3-axis accelerometer driver
#
# Copyright (c) 2020 STMicroelectronics
# Copyright (c) 2024 SILA Embedded Solutions GmbH
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(iis328dq.c)
zephyr_library_sources_ifdef(CONFIG_IIS328DQ_TRIGGER    iis328dq_trigger.c)

zephyr_library_include_directories(../stmemsc)
