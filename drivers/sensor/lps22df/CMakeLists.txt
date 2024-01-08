# ST Microelectronics LPS22DF pressure and temperature sensor
#
# Copyright (c) 2023 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lps22df.c)
zephyr_library_sources_ifdef(CONFIG_LPS22DF_TRIGGER    lps22df_trigger.c)

zephyr_library_include_directories(../stmemsc)
