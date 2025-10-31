# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(.)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)

zephyr_library_sources_ifdef(CONFIG_QUIC quic.c)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
