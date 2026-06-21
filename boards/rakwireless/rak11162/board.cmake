# Copyright (c) 2026 RAKwireless Technology Limited <www.rakwireless.com>
# Sercan Erat <sercanerat@gmail.com>
# JaeHwan Jin <jaehwan.jin@rakwireless.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_RAK11162_STM32WLE5XX)
  board_runner_args(pyocd "--target=stm32wle5ccux")
  board_runner_args(pyocd "--flash-opt=-O cmsis_dap.limit_packets=1")
  board_runner_args(jlink "--device=STM32WLE5CC" "--speed=4000" "--reset-after-load")
  board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

  include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)

elseif(CONFIG_BOARD_RAK11162_ESP32C2)
  board_set_flasher_ifnset(esp32)

  set(ESP_IDF_PATH ${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR})
  assert(ESP_IDF_PATH "ESP_IDF_PATH is not set")

  board_runner_args(esp32 "--esp-idf-path=${ESP_IDF_PATH}")
endif()
