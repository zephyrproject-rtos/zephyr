# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(wireguard)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources(wg.c)

zephyr_library_sources_ifdef(CONFIG_BOARD_NATIVE_SIM ns_rtc.c)
