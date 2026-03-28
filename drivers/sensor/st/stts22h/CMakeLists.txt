# ST Microelectronics STTS22H temperature sensor
#
# Copyright (c) 2024 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(stts22h.c)
zephyr_library_sources_ifdef(CONFIG_STTS22H_TRIGGER    stts22h_trigger.c)

zephyr_library_include_directories(../stmemsc)
