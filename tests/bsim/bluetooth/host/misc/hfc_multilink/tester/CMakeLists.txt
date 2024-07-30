# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hfc_multilink_tester)

target_sources(app PRIVATE src/main.c src/tester.c)

# This contains babblesim-specific helpers, e.g. device synchronization.
add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
  ../
  ${ZEPHYR_BASE}/subsys/bluetooth/common/
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )
