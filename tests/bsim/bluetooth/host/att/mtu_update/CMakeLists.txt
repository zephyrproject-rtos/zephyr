# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_mtu_update)

add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

target_include_directories(app PRIVATE src)

target_sources_ifdef(CONFIG_BT_PERIPHERAL app PRIVATE
  src/main_peripheral.c
  ${ZEPHYR_BASE}/samples/bluetooth/mtu_update/peripheral/src/peripheral_mtu_update.c
)

target_sources_ifdef(CONFIG_BT_CENTRAL app PRIVATE
  src/main_central.c
  ${ZEPHYR_BASE}/samples/bluetooth/mtu_update/central/src/central_mtu_update.c
)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
