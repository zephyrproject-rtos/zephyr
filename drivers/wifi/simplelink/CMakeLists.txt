# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_SIMPLELINK)
  zephyr_include_directories(
    ${ZEPHYR_BASE}/subsys/net/lib/tls_credentials
    )
  zephyr_sources(
    simplelink_support.c
    simplelink.c
    )
endif()
zephyr_sources_ifdef(CONFIG_NET_SOCKETS_OFFLOAD simplelink_sockets.c)
