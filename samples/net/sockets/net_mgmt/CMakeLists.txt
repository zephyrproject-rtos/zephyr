# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sockets_net_mgmt)

target_sources_ifdef(CONFIG_NET_SOCKETS_NET_MGMT app PRIVATE src/main.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
