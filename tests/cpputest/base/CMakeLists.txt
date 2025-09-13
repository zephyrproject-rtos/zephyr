# Copyright (c) 2025, Victor Chavez (vchavezb@protonmail.com)
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cpputest_sample)

include(FetchContent)
FetchContent_Declare(
    CppUTest
    GIT_REPOSITORY https://github.com/cpputest/cpputest.git
    GIT_TAG        v4.0
)
set(TESTS OFF CACHE BOOL "Switch off CppUTest Test build")

FetchContent_MakeAvailable(CppUTest)


target_sources(app PRIVATE  src/main.cpp
                            src/test_suite.cpp
)

target_link_libraries(app PRIVATE
                            CppUTest
                            CppUTestExt)
