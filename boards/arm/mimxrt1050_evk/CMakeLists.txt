#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

target_sources(board PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pinmux.c)
target_include_directories(board PRIVATE ${ZEPHYR_BASE}/drivers)
