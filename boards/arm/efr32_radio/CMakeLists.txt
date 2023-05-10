# SPDX-License-Identifier: Apache-2.0

if(CONFIG_UART_GECKO)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()

zephyr_include_directories(.)
