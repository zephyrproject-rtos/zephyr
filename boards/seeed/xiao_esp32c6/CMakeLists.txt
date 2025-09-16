#
# Copyright (c) 2025 Mario Paja
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_XIAO_ESP32C6_EXT_ANTENNA)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
