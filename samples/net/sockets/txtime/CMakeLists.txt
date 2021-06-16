# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(sockets_txtime)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_NET_VLAN app PRIVATE src/vlan.c)
