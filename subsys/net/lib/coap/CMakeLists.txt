# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_COAP
  coap.c
  coap_link_format.c
)

zephyr_sources_ifdef(CONFIG_COAP_CLIENT
  coap_client.c
)

zephyr_sources_ifdef(CONFIG_COAP_SERVER
  coap_server.c
)

if(CONFIG_COAP_SERVER AND CONFIG_MBEDTLS_BUILTIN)
  # coap_server.c is built into the "zephyr" library.
  target_link_libraries(zephyr PUBLIC tfpsacrypto)
endif()

zephyr_sources_ifdef(CONFIG_COAP_SERVER_SHELL
  coap_server_shell.c
)

zephyr_sources_ifdef(CONFIG_COAP_CLIENT_TCP
  coap_client_tcp.c
)

zephyr_sources_ifdef(CONFIG_COAP_OSCORE
  coap_oscore.c
)
