# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

add_subdirectory_ifdef(CONFIG_WIFI_ESP_AT       esp_at)
add_subdirectory_ifdef(CONFIG_WIFI_ESP32        esp32)
add_subdirectory_ifdef(CONFIG_WIFI_ESWIFI       eswifi)
add_subdirectory_ifdef(CONFIG_WIFI_SIMPLELINK   simplelink)
add_subdirectory_ifdef(CONFIG_WIFI_WINC1500     winc1500)
