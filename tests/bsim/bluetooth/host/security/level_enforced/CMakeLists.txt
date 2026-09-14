# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS "$ENV{ZEPHYR_BASE}")
project(bsim_test_security_level_enforced)

add_subdirectory("${ZEPHYR_BASE}/tests/bsim/babblekit" babblekit)
target_link_libraries(app PRIVATE babblekit)

target_sources(app PRIVATE
  src/central.c
  src/common.c
  src/main.c
  src/peripheral.c
)

zephyr_include_directories(
  "${BSIM_COMPONENTS_PATH}/libUtilv1/src/"
  "${BSIM_COMPONENTS_PATH}/libPhyComv1/src/"
)
