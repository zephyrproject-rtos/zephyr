# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(peripheral_hids)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
  ${app_sources}
  )

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)

add_subdirectory(../gatt gatt_bindir)

zephyr_library_cmake_target(samples_bt_gatt gatt_tgt)
target_link_libraries(app PRIVATE ${gatt_tgt})
