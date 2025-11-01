# ST Microelectronics stmemsc hal i/f
#
# Copyright (c) 2021 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(sensirion_common.c)
zephyr_library_sources(sensirion_i2c.c)
zephyr_library_sources(crc_tables.c)
