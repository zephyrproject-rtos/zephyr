# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nanopb_sample)

nanopb_generate_cpp(proto_sources proto_headers RELPATH .
    src/simple.proto
)
# we need to be able to include generated header files
zephyr_library_include_directories(${CMAKE_CURRENT_BINARY_DIR})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${proto_sources} ${app_sources})
