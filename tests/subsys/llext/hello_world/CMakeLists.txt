# Copyright (c) 2023 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

# TODO check which architecture is being used
set(CMAKE_C_FLAGS "-mlong-calls" "-mthumb")

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/hello_world.llext
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -c -o ${PROJECT_BINARY_DIR}/hello_world.llext ${PROJECT_SOURCE_DIR}/hello_world.c
)

set(HELLO_WORLD_LLEXT ${PROJECT_BINARY_DIR}/hello_world.llext PARENT_SCOPE)

add_custom_target(hello_world DEPENDS ${PROJECT_BINARY_DIR}/hello_world.llext)
