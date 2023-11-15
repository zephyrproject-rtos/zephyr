# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mdns_responder)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/service.c)
target_sources_ifdef(CONFIG_NET_VLAN app PRIVATE src/vlan.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
