#
# Copyright (c) 2018, Cypress
#
# SPDX-License-Identifier: Apache-2.0
#

target_sources(board PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pinmux.c)
target_include_directories(board PRIVATE ${PROJECT_SOURCE_DIR}/drivers)
