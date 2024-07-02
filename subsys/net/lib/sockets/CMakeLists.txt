# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/net/socket.h
  ${ZEPHYR_BASE}/include/zephyr/net/socket_select.h
)

zephyr_library_include_directories(.)

zephyr_library_sources(
  getaddrinfo.c
  sockets.c
  sockets_select.c
)

if(NOT CONFIG_NET_SOCKETS_OFFLOAD)
zephyr_library_sources(
  getnameinfo.c
  sockets_misc.c
  )
endif()

zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_CAN                sockets_can.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_PACKET             sockets_packet.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_SOCKOPT_TLS        sockets_tls.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_OFFLOAD            socket_offload.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_OFFLOAD_DISPATCHER socket_dispatcher.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_OBJ_CORE           socket_obj_core.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_SERVICE            sockets_service.c)

if(CONFIG_NET_SOCKETS_NET_MGMT)
  zephyr_library_sources(sockets_net_mgmt.c)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
endif()

if(CONFIG_SOCKS)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/socks)
endif()

zephyr_library_sources_ifdef(CONFIG_NET_SOCKETPAIR socketpair.c)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
