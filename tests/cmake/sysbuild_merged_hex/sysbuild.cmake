#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

ExternalZephyrProject_Add(
  APPLICATION other_hello
  SOURCE_DIR ${ZEPHYR_BASE}/samples/hello_world
  BOARD nrf9160dk/nrf52840
)

add_custom_command(
  OUTPUT
  ${CMAKE_BINARY_DIR}/test1.hex
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/mergehex.py
  -o ${CMAKE_BINARY_DIR}/test1.hex
  --overlap replace
  mcuboot/zephyr/zephyr.hex
  sysbuild_merged_hex/zephyr/zephyr.signed.hex
  DEPENDS
  mcuboot_extra_byproducts;sysbuild_merged_hex_extra_byproducts;merged_nrf9160dk_0_14_0_nrf9160_target
  WORKING_DIRECTORY
  ${CMAKE_BINARY_DIR}
)

add_custom_target(
  test1_target
  ALL DEPENDS
  ${CMAKE_BINARY_DIR}/test1.hex
)

add_custom_command(
  OUTPUT
  ${CMAKE_BINARY_DIR}/test2.hex
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/mergehex.py
  -o ${CMAKE_BINARY_DIR}/test2.hex
  --overlap replace
  other_hello/zephyr/zephyr.hex
  DEPENDS
  other_hello_extra_byproducts;merged_nrf9160dk_0_14_0_nrf52840_target
  WORKING_DIRECTORY
  ${CMAKE_BINARY_DIR}
)

add_custom_target(
  test2_target
  ALL DEPENDS
  ${CMAKE_BINARY_DIR}/test2.hex
)

add_custom_command(
  OUTPUT
  test3_result
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${APP_DIR}/compare_output.py
  DEPENDS
  test1_target;test2_target;${CMAKE_BINARY_DIR}/test1.hex;${CMAKE_BINARY_DIR}/test2.hex
  WORKING_DIRECTORY
  ${CMAKE_BINARY_DIR}
)

add_custom_target(
  test3_target
  ALL DEPENDS
  test3_result
)
