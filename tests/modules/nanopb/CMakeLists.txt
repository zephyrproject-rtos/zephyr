# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nanopb_tests)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

zephyr_nanopb_sources(app proto/simple.proto)

zephyr_nanopb_sources(app
  proto/complex.proto
  proto/sub/nested.proto
)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Process our own library
add_subdirectory(lib)
target_include_directories(app PRIVATE lib)
target_link_libraries(app PRIVATE mylib)
