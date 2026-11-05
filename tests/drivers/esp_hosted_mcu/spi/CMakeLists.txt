# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esp_hosted_mcu_spi)

target_sources(app PRIVATE
  src/main.c
)
target_include_directories(app PRIVATE
  "${ZEPHYR_BASE}/drivers/misc/esp_hosted_mcu"
)
# Compile the transport in isolation, with its hardware calls replaced by mocks.
target_compile_definitions(app PRIVATE
  CONFIG_ESP_HOSTED_MCU_LOG_LEVEL=3
  CONFIG_ESP_HOSTED_MCU_RESET_PULSE_MS=0
  CONFIG_ESP_HOSTED_MCU_SPI_RESET_SETTLE_MS=0
)

list(APPEND CMAKE_MODULE_PATH "${ZEPHYR_BASE}/modules/nanopb")
include(nanopb)
zephyr_nanopb_sources(app
  "${ZEPHYR_BASE}/drivers/misc/esp_hosted_mcu/esp_hosted_mcu_rpc.proto"
)
