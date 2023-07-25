# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Oticon A/S

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral_test)

FILE(GLOB app_sources src/*.c)
FILE(GLOB gatt_sources src/gatt/*.c)
target_sources(app PRIVATE
  ${app_sources}
  ../common/edtt_driver_bsim.c
  ${gatt_sources}
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth
  ../common/
  src/
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
