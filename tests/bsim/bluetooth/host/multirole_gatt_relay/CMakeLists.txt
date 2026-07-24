# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_multirole_gatt_relay)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/samples/bluetooth/peripheral_gatt_notify/src/peripheral_gatt_notify.c
  ${ZEPHYR_BASE}/samples/bluetooth/central_notify_receive/src/central_notify_receive.c
  ${ZEPHYR_BASE}/samples/bluetooth/central_peripheral_notify_send_receive/src/central_peripheral_notify.c
)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
