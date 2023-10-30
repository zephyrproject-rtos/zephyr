# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(unicast_audio_client_self_tets)

set(unicast_client_path ${ZEPHYR_BASE}/samples/bluetooth/unicast_audio_client)

target_sources(app PRIVATE
  ${unicast_client_path}/src/main.c
)

target_sources(app PRIVATE
	src/unicast_client_sample_test.c
	src/test_main.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )
