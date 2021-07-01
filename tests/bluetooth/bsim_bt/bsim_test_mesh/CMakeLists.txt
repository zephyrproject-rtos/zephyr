# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if (NOT DEFINED ENV{BSIM_COMPONENTS_PATH})
	message(FATAL_ERROR "This test requires the BabbleSim simulator. Please set\
 the  environment variable BSIM_COMPONENTS_PATH to point to its components \
 folder. More information can be found in\
 https://babblesim.github.io/folder_structure_and_env.html")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_mesh)

target_sources(app PRIVATE
 src/main.c
 src/mesh_test.c
)

if(CONFIG_SETTINGS)
 target_sources(app PRIVATE
  src/settings_test_backend.c
  src/test_persistence.c
 )
else()
 target_sources(app PRIVATE
  src/test_transport.c
  src/test_friendship.c
  src/test_provision.c
  src/test_beacon.c
 )
endif()

zephyr_include_directories(
  $ENV{BSIM_COMPONENTS_PATH}/libUtilv1/src/
  $ENV{BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
