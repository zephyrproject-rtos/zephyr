# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(NOT CONFIG_NET_SOCKETS_OFFLOAD)
zephyr_sources(
  getaddrinfo.c
  getnameinfo.c
  sockets.c
  sockets_select.c
  sockets_misc.c
  )
zephyr_sources_ifdef(CONFIG_NET_SOCKETS_SOCKOPT_TLS sockets_tls.c)
zephyr_sources_ifdef(CONFIG_NET_SOCKETS_PACKET sockets_packet.c)
zephyr_sources_ifdef(CONFIG_NET_SOCKETS_CAN sockets_can.c)
endif()
zephyr_sources_ifdef(CONFIG_NET_SOCKETS_OFFLOAD     socket_offload.c)

if(CONFIG_NET_SOCKETS_NET_MGMT)
  zephyr_sources(sockets_net_mgmt.c)
  zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
endif()

if(CONFIG_SOCKS)
  zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/lib/socks)
endif()

zephyr_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
