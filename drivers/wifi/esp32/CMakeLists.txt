# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_WIFI_ESP32
  src/esp_wifi_drv.c
  )

zephyr_library_sources_ifdef(CONFIG_WIFI_ESP32_MESH
  src/esp_wifi_mesh.c
  )

zephyr_library_sources_ifdef(CONFIG_WIFI_ESP32_MESH_IP
  src/esp_wifi_mesh_netif.c
  )

if(CONFIG_WIFI_ESP32 AND CONFIG_ESP_WIFI_HEAP_SYSTEM AND NOT CONFIG_HEAP_MEM_POOL_IGNORE_MIN)
  # Default of CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM, covered by HEAP_MEM_POOL_ADD_SIZE_ESP_WIFI
  set(esp_wifi_default_static_rx 10)
  # Approximate size of one Wi-Fi static RX buffer
  set(esp_wifi_rx_buffer_size 1600)
  math(EXPR esp_wifi_extra_rx
    "${CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM} - ${esp_wifi_default_static_rx}")

  if(esp_wifi_extra_rx GREATER 0)
    import_kconfig(CONFIG_HEAP_MEM_POOL_ADD_SIZE_ "${DOTCONFIG}" esp_wifi_add_size_keys)

    set(esp_wifi_heap_min 0)
    foreach(esp_wifi_add_size ${esp_wifi_add_size_keys})
      math(EXPR esp_wifi_heap_min "${esp_wifi_heap_min} + ${${esp_wifi_add_size}}")
    endforeach()
    math(EXPR esp_wifi_heap_min
      "${esp_wifi_heap_min} + ${esp_wifi_extra_rx} * ${esp_wifi_rx_buffer_size}")

    if(CONFIG_HEAP_MEM_POOL_SIZE LESS esp_wifi_heap_min)
      message(WARNING "
      Heap too small for Wi-Fi RX buffers, throughput will drop:
        CONFIG_HEAP_MEM_POOL_SIZE=${CONFIG_HEAP_MEM_POOL_SIZE} < ${esp_wifi_heap_min} needed for
        CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=${CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM}
      Increase CONFIG_HEAP_MEM_POOL_SIZE or reduce CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM.")
    endif()
  endif()
endif()
