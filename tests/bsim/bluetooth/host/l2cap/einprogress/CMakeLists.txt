# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(test_l2cap_einprogress)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/common/testlib testlib)
target_link_libraries(app PRIVATE testlib)

add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)

target_sources(app PRIVATE
  src/main.c
  src/dut.c
  src/tester.c
)
