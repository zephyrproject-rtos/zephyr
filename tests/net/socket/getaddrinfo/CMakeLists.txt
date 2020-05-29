# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(getaddrinfo)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/dns)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
