# Copyright (c) 2025 Arduino SA
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_WIFI_ESP_HOSTED)

zephyr_library_named(esp_hosted)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

zephyr_library_sources(
  esp_hosted_hal.c
  esp_hosted_wifi.c
)

zephyr_nanopb_sources(esp_hosted
  esp_hosted_proto.pb
)

zephyr_include_directories(
  ./
)

zephyr_compile_definitions(
    PB_ENABLE_MALLOC=1
)
endif()
