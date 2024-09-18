# Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_SERIES_ESP32 OR CONFIG_SOC_SERIES_ESP32S2 OR CONFIG_SOC_SERIES_ESP32S3)
  zephyr_include_directories(include)
endif()

zephyr_sources_ifdef(CONFIG_ESP_SPIRAM psram.c)
