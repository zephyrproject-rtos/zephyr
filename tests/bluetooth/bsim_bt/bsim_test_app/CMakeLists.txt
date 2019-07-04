# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if (NOT DEFINED ENV{BSIM_COMPONENTS_PATH})
	message(FATAL_ERROR "This test requires the BabbleSim simulator. Please set\
 the  environment variable BSIM_COMPONENTS_PATH to point to its components \
 folder. More information can be found in\
 https://babblesim.github.io/folder_structure_and_env.html")
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(bsim_test_app)

target_sources(app PRIVATE
	src/main.c
	src/test_empty.c
	src/test_connect1.c
	src/test_connect2.c
)

zephyr_include_directories(
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  $ENV{ZEPHYR_BASE}/samples/bluetooth
)
