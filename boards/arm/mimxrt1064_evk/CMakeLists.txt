#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_sources(pinmux.c)

if (CONFIG_DISPLAY)
message(WARNING "
CONFIG_DISPLAY: Running this firmware on a board without a display may damage the board
")
endif()
