#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(soc.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
