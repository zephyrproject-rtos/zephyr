# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mcp_tests)

# Main test file
target_sources(app PRIVATE src/main.c)


target_include_directories(app PRIVATE
    ${ZEPHYR_BASE}/subsys/net/lib/mcp
    ${ZEPHYR_BASE}/include/zephyr/net/mcp
)
