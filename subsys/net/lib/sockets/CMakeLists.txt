zephyr_include_directories(.)

if(NOT CONFIG_NET_SOCKETS_OFFLOAD)
zephyr_sources(
  getaddrinfo.c
  sockets.c
  )
zephyr_sources_ifdef(CONFIG_NET_SOCKETS_SOCKOPT_TLS sockets_tls.c)
endif()
zephyr_sources_ifdef(CONFIG_NET_SOCKETS_OFFLOAD     socket_offload.c)
