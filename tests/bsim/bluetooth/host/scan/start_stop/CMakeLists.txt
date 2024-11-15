# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_scan_start_stop)

target_sources(app PRIVATE
  src/main.c
)

# This contains babblesim-specific helpers, e.g. device synchronization.
add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
        $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
        $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
