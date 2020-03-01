# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

if(NOT EXISTS ${APPLICATION_SOURCE_DIR}/src/private_info/key.c)
  message(FATAL_ERROR "!!!!!! Generate key file before continuing.  See README !!!!!")
endif()

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE ${APPLICATION_SOURCE_DIR}/src/private_info/key.c)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/tls_config)
