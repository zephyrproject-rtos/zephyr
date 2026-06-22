# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ocpp)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
target_sources(app PRIVATE src/main.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

if(CONFIG_NET_SOCKETS_SOCKOPT_TLS AND (NOT USE_DUMMY_CREDS))
generate_inc_file_for_target(
    app
    # <path-to-certificate-file> (e.g. src/credentials/device.crt)
    # <path-to-generated-.inc-file> (e.g. ${gen_dir}/device.crt.inc)
)
endif()
