# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(st_ble_802154_echo_client)

target_sources(                     app PRIVATE src/echo-client.c)
target_sources(                     app PRIVATE src/app_ble.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE src/udp.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
