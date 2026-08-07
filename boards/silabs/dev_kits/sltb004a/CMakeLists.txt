# Copyright (c) 2020 Christian Taedcke
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CCS811)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
