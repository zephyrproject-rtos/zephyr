# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iface)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include(gen_count)

gen_section_count_add_consumer(
    TARGET  app
    HEADER  net_if
    FILES   ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)
