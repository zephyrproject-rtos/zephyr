zephyr_library()

zephyr_library_sources(
  zstream.c
  zstream_sock.c
  )

zephyr_library_sources_ifdef(CONFIG_MBEDTLS zstream_tls.c)

zephyr_link_interface_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
