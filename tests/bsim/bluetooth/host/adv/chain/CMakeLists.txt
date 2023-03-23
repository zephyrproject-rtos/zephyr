# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_adv_chain)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/samples/bluetooth/broadcaster_multiple/src/broadcaster_multiple.c
  ${ZEPHYR_BASE}/samples/bluetooth/observer/src/observer.c
)

zephyr_include_directories(
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
