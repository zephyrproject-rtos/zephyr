# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_ble_lvgl)

target_sources(app PRIVATE 
    src/main.c
    src/wifi_manager.c
    src/ble_gatt.c
    src/nvs_storage.c
    src/ui_manager.c
)
