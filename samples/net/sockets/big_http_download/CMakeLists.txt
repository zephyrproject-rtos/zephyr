# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(big_http_download)

target_sources(app PRIVATE src/big_http_download.c)
target_sources_ifdef(CONFIG_WIFI app PRIVATE src/wifi.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    src/lets-encrypt-r3.der
    ${gen_dir}/lets-encrypt-r3.der.inc
    )
