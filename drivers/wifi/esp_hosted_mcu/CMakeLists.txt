# Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_ESP_HOSTED_MCU)

  zephyr_library_named(esp_hosted_mcu)

  zephyr_library_sources(
    esp_hosted_mcu.c
  )

  # Export the header so an application can re-run the bring-up after the
  # coprocessor restarted, as the transport core exports its own.
  zephyr_include_directories(./)

endif()
