# SPDX-License-Identifier: Apache-2.0

if(CONFIG_GPIO_ESP32 AND CONFIG_SOC_ESP32_PROCPU)
  zephyr_library()
  zephyr_library_sources(board_init.c)
endif()
