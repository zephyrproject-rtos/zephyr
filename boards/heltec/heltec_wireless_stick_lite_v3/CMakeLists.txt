# SPDX-License-Identifier: Apache-2.0

if(CONFIG_GPIO_ESP32)
  zephyr_library()
  zephyr_library_sources(board_init.c)
endif()
