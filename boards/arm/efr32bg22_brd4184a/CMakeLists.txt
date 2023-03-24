# Copyright (c) 2021 Sateesh Kotapati
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_UART_GECKO)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
