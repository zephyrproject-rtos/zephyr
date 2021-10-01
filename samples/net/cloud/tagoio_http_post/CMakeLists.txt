# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tagoio_http_post)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/sockets.c)
target_sources_ifdef(CONFIG_WIFI app PRIVATE src/wifi.c)
