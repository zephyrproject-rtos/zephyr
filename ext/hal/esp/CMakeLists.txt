# At the moment there is only one supported ESP32 SoC, and it is not
# supported to omit the esp-dif HAL library, so we don't check for a
# KConfig option to enable the HAL
#
# In the future it might look like this:
# if(CONFIG_ESP_DIF_LIBRARY)
# to allow users to disable the HAL
if(CONFIG_SOC_ESP32)
  set_ifndef(ESP_IDF_PATH $ENV{ESP_IDF_PATH})
  set(       ESP_IDF_PATH ${ESP_IDF_PATH} CACHE PATH "")
  assert(    ESP_IDF_PATH "ESP_IDF_PATH is not set")

  zephyr_include_directories(
    ${ESP_IDF_PATH}/components/esp32/include
    ${ESP_IDF_PATH}/components/soc/esp32/include
    )

  zephyr_link_libraries(
    hal
    -L${ESP_IDF_PATH}/components/esp32
    )
endif()
