# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dumb_http_server)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(app src/response_small.html.bin ${gen_dir}/response_small.html.bin.inc)
generate_inc_file_for_target(app src/response_big.html.bin   ${gen_dir}/response_big.html.bin.inc)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
