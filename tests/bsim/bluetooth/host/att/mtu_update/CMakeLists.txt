# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_mtu_update)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/samples/bluetooth/mtu_update/central/src/central_mtu_update.c
  ${ZEPHYR_BASE}/samples/bluetooth/mtu_update/peripheral/src/peripheral_mtu_update.c
)

zephyr_include_directories(
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
