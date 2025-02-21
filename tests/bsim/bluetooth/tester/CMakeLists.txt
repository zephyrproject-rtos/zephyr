# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tester_bsim)

# This contains babblesim-specific helpers, e.g. device synchronization.
add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )

zephyr_include_directories(
  src/
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/audio/btp
)

target_sources(app PRIVATE
  src/bsim_btp.c
  src/test_main.c
  src/host/gap_central.c
  src/host/gap_peripheral.c
)
