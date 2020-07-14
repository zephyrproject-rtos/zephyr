# Copyright (c) 2020 Christian Taedcke
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CCS811)
  zephyr_library()
  zephyr_library_sources(board.c)
  zephyr_library_include_directories(${PROJECT_SOURCE_DIR}/drivers)
endif()
