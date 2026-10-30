# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(websocket)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/websocket
)
target_sources(app PRIVATE src/handshake.c)
target_sources_ifdef(CONFIG_NET_TEST app PRIVATE src/main.c)
target_sources_ifndef(CONFIG_NET_TEST app PRIVATE src/loopback.c)
