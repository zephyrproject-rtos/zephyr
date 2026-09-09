# Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ESP_HOSTED_MCU)

  zephyr_library_named(esp_hosted_mcu_core)

  list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
  include(nanopb)

  zephyr_library_sources(
    esp_hosted_mcu.c
  )

  zephyr_library_sources_ifdef(CONFIG_ESP_HOSTED_MCU_SPI  esp_hosted_mcu_spi.c)
  zephyr_library_sources_ifdef(CONFIG_ESP_HOSTED_MCU_SDIO esp_hosted_mcu_sdio.c)

  zephyr_nanopb_sources(esp_hosted_mcu_core
    esp_hosted_mcu_rpc.proto
  )

  # Export the public header and the nanopb-generated RPC header so protocol
  # consumers (Wi-Fi netif, Bluetooth HCI) can include them.
  zephyr_include_directories(
    ./
    ${CMAKE_CURRENT_BINARY_DIR}
  )

endif()
