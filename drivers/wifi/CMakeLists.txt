# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

if (CONFIG_BUILD_ONLY_NO_BLOBS)
  message(WARNING "
  ---------------------------------------------------------------------------
  Building only the Wi-Fi driver without binary blobs and patches.
  This is only for building (CI) purposes and will not work on a real device.
  ---------------------------------------------------------------------------
  ")
else()

if(CONFIG_DT_HAS_ESPRESSIF_ESP32_WIFI_ENABLED)
  zephyr_blobs_verify(MODULE hal_espressif REQUIRED)
endif()

endif() # CONFIG_BUILD_ONLY_NO_BLOBS

add_subdirectory_ifdef(CONFIG_WIFI_ESP_AT       esp_at)
add_subdirectory_ifdef(CONFIG_WIFI_ESP32        esp32)
add_subdirectory_ifdef(CONFIG_WIFI_ESWIFI       eswifi)
add_subdirectory_ifdef(CONFIG_WIFI_SIMPLELINK   simplelink)
add_subdirectory_ifdef(CONFIG_WIFI_WINC1500     winc1500)
add_subdirectory_ifdef(CONFIG_WIFI_NXP          nxp)
add_subdirectory_ifdef(CONFIG_WIFI_AIROC        infineon)
add_subdirectory_ifdef(CONFIG_WIFI_NRF70        nrf_wifi)
add_subdirectory_ifdef(CONFIG_WIFI_SILABS_SIWX91X siwx91x)
