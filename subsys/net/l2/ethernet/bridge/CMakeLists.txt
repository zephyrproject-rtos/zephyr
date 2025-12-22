# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources(bridge.c)
zephyr_library_sources(bridge_input.c)
zephyr_library_sources_ifdef(CONFIG_NET_ETHERNET_BRIDGE_SHELL bridge_shell.c)
