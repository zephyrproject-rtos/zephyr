# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esp_tool_demo)

target_sources(app PRIVATE src/main.c)

if (CONFIG_ESP_TOOL_SHELL)
  target_sources(app PRIVATE src/shell.c)
endif()

if (CONFIG_ESP_TOOL_FW_ARRAYS)
  create_resources(${CMAKE_CURRENT_SOURCE_DIR}/target-firmware
                   ${CMAKE_CURRENT_SOURCE_DIR}/res/target_firmware_data.c)
  target_sources(app PRIVATE res/target_firmware_data.c)
endif()
