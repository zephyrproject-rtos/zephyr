zephyr_include_directories(.)
zephyr_sources(
  getaddrinfo.c
  sockets.c
  )

zephyr_sources_ifdef(CONFIG_NET_SOCKETS_SOCKOPT_TLS   sockets_tls.c)
zephyr_link_interface_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
