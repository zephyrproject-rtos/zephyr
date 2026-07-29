# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(quectel-bg9x.c)

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/modem)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)
