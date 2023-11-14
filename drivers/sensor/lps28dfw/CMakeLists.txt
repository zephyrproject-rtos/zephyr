# ST Microelectronics LPS28DFW pressure and temperature sensor
#
# Copyright (c) 2023 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lps28dfw.c)
zephyr_library_sources_ifdef(CONFIG_LPS28DFW_TRIGGER    lps28dfw_trigger.c)

zephyr_library_include_directories(../stmemsc)
