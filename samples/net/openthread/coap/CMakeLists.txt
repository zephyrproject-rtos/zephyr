# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ot_coap)

target_include_directories(app PRIVATE src)

target_sources(app PRIVATE
    src/main.c
    src/coap_utils.c
    src/coap_shell.c
)

target_sources_ifdef(CONFIG_OT_COAP_SAMPLE_LED app PRIVATE src/led.c)
target_sources_ifdef(CONFIG_OT_COAP_SAMPLE_SW app PRIVATE src/button.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
