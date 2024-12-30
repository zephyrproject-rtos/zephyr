# Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)

if(NOT CONFIG_MCUBOOT AND NOT CONFIG_SOC_ESP32_APPCPU AND NOT CONFIG_SOC_ESP32S3_APPCPU)
  zephyr_sources_ifdef(CONFIG_ESP_HEAP_RUNTIME esp_heap_runtime.c)
  zephyr_sources_ifdef(CONFIG_ESP_SPIRAM esp_psram.c)
endif()
