# Copyright (c) 2021 Sateesh Kotapati
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_UART_GECKO)
  zephyr_library()
  zephyr_library_sources(board.c)

endif()

# to include sl_device_init_hfxo_config.h
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
