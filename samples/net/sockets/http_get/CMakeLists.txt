# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(http_get)

target_sources(app PRIVATE src/http_get.c)
target_sources_ifdef(CONFIG_WIFI app PRIVATE src/wifi.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    src/globalsign_r1.der
    ${gen_dir}/globalsign_r1.der.inc
    )
