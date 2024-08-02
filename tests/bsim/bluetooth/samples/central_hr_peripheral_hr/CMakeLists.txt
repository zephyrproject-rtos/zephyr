# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_hr_peripheral_hr_test)

set(central_hr_path ${ZEPHYR_BASE}/samples/bluetooth/central_hr)

target_sources(app PRIVATE
  ${central_hr_path}/src/main.c
)

target_sources(app PRIVATE
	src/sample_test.c
	src/test_main.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )
