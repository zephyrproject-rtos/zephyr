#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_include_directories(${PROJECT_SOURCE_DIR}/drivers)
zephyr_library_sources(pinmux.c)
