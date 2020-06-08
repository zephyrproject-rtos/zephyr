# Copyright (c) 2020 InnBlue
# SPDX-License-Identifier: Apache-2.0
zephyr_library()
zephyr_library_sources(innblue21_board_init.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
