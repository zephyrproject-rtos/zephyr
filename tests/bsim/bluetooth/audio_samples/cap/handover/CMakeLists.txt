# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cap_handover_self_tests)

set(cap_handover_path ${ZEPHYR_BASE}/samples/bluetooth/cap_handover)

target_sources(app PRIVATE
	src/cap_handover_sample_test.c
	src/test_main.c

  ${cap_handover_path}/src/main.c
  ${cap_handover_path}/src/cap_stream_tx.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )
