# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dhcpv6_server)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/ip
  ${ZEPHYR_BASE}/subsys/net/lib/dhcpv6)
target_sources(app PRIVATE src/main.c)

zephyr_compile_definitions(CONFIG_NET_DHCPV6_SERVER)
zephyr_compile_definitions(CONFIG_NET_DHCPV6_SERVER_LOG_LEVEL=LOG_LEVEL_ERR)
zephyr_compile_definitions(CONFIG_NET_DHCPV6_SERVER_MAX_LEASES=4)
zephyr_compile_definitions(CONFIG_NET_DHCPV6_SERVER_VALID_LIFETIME=86400)
zephyr_compile_definitions(CONFIG_NET_DHCPV6_SERVER_PREFERRED_LIFETIME=43200)
