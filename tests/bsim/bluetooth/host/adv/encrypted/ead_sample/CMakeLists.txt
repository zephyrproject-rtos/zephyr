# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_ead_sample)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/samples/bluetooth/encrypted_advertising/central/src/central_ead.c
  ${ZEPHYR_BASE}/samples/bluetooth/encrypted_advertising/peripheral/src/peripheral_ead.c
)

zephyr_include_directories(
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/

  ${ZEPHYR_BASE}/samples/bluetooth/encrypted_advertising/peripheral/src/
  ${ZEPHYR_BASE}/samples/bluetooth/encrypted_advertising/include/
)
