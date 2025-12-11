# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(websocket_client)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)

zephyr_library_sources(
  websocket.c
)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
