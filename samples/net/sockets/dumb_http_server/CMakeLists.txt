# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(dumb_http_server)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(app src/response_small.html.bin ${gen_dir}/response_small.html.bin.inc)
generate_inc_file_for_target(app src/response_big.html.bin   ${gen_dir}/response_big.html.bin.inc)

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)
