# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ccp_call_control_client_self_tets)

set(ccp_call_control_client_path ${ZEPHYR_BASE}/samples/bluetooth/ccp_call_control_client)

target_sources(app PRIVATE
  ${ccp_call_control_client_path}/src/main.c
)

target_sources(app PRIVATE
	src/test_main.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )
