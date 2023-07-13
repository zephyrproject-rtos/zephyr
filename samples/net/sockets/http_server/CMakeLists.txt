# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Python REQUIRED COMPONENTS Interpreter)

project(http_server_with_poll)

target_sources(app PRIVATE src/server.c src/main.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
message(STATUS "gen_dir is set to ${gen_dir}")

set(source_file src/index.html)

generate_inc_file_for_target(app ${source_file} ${gen_dir}/index.html.gz.inc --gzip)

target_link_libraries(app PRIVATE zephyr_interface zephyr)
