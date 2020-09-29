# Copyright (c) 2020 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_sources(board.c)
zephyr_include_directories(.)
