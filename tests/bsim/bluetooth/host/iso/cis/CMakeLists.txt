# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_iso_cis)

add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

target_sources(app PRIVATE
  src/common.c
  src/cis_central.c
  src/cis_peripheral.c
  src/main.c
)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/common/testlib testlib)

target_link_libraries(app PRIVATE
  testlib
  babblekit
)
