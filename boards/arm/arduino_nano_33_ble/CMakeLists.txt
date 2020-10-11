# Copyright (c) 2020 Jefferson Lee.
# SPDX-License-Identifier: Apache-2.0

add_library(arduino_nano_33_ble_pins INTERFACE)
target_include_directories(arduino_nano_33_ble_pins 
        INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/inc/")


if(CONFIG_BOARD_ARDUINO_NANO_33_BLE_INIT_SENSORS)
  zephyr_library()
  zephyr_library_sources("${CMAKE_CURRENT_SOURCE_DIR}/src/init_sensors.c")
  target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE arduino_nano_33_ble_pins)
endif()
