# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources_ifdef(CONFIG_NET_DHCPV6 dhcpv6.c)
zephyr_library_sources_ifdef(CONFIG_NET_DHCPV6_SERVER dhcpv6_server.c)
