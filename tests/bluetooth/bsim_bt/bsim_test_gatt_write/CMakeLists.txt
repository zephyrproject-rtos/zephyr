# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (NOT DEFINED ENV{BSIM_COMPONENTS_PATH})
  message(FATAL_ERROR "This test requires the BabbleSim simulator. Please set  \
          the  environment variable BSIM_COMPONENTS_PATH to point to its       \
          components folder. More information can be found in                  \
          https://babblesim.github.io/folder_structure_and_env.html")
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_gatt_write)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/samples/bluetooth/central_gatt_write/src/gatt_write_common.c
  ${ZEPHYR_BASE}/samples/bluetooth/central_gatt_write/src/central_gatt_write.c
  ${ZEPHYR_BASE}/samples/bluetooth/peripheral_gatt_write/src/peripheral_gatt_write.c
)

zephyr_include_directories(
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
