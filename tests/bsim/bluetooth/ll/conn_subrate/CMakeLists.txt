# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_conn_subrate)

target_sources(app PRIVATE
  src/main.c
  src/central.c
  src/peripheral.c
)

# The bsim test framework headers (bstests.h, bs_types.h, bs_tracing.h,
# time_machine.h) live in the BabbleSim components; the board only adds these
# to its own library scope, so app sources need them added globally.
zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
