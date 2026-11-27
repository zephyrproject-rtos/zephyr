# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap_server_iface)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
target_sources(app PRIVATE src/main.c)

zephyr_linker_sources(DATA_SECTIONS sections-ram.ld)
