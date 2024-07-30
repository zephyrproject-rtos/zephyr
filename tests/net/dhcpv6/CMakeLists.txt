# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dhcpv6)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
target_sources(app PRIVATE src/main.c)

zephyr_compile_definitions(CONFIG_NET_DHCPV6)
zephyr_compile_definitions(CONFIG_NET_DHCPV6_LOG_LEVEL=LOG_LEVEL_ERR)
