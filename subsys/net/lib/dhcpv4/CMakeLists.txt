# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources_ifdef(CONFIG_NET_DHCPV4 dhcpv4.c)
zephyr_library_sources_ifdef(CONFIG_NET_DHCPV4_SERVER dhcpv4_server.c)
