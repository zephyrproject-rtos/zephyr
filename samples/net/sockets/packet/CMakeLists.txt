# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sockets_echo_server)

target_sources(                     app PRIVATE src/packet.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
