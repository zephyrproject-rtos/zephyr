# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

project(buf_simple)

add_subdirectory(${ZEPHYR_BASE}/tests/net/mocks net_mocks)

target_link_libraries(testbinary PRIVATE net_mocks)

target_sources(testbinary
    PRIVATE
    src/main.c

    ${ZEPHYR_BASE}/subsys/net/buf_simple.c
)
