# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lwm2m_objects)

# One source file per object under test, each with its own ztest suite
file(GLOB app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/lwm2m)
