# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_WIFI_ESP32
  src/esp_wifi_drv.c
  )
