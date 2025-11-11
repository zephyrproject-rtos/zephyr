#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
