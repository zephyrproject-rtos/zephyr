# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_COAP
  coap.c
  coap_link_format.c
)

zephyr_library_sources_ifdef(CONFIG_COAP_CLIENT
  coap_client.c
)

zephyr_library_sources_ifdef(CONFIG_COAP_SERVER
  coap_server.c
)

zephyr_library_sources_ifdef(CONFIG_COAP_SERVER_SHELL
  coap_server_shell.c
)

zephyr_library_sources_ifdef(CONFIG_COAP_CLIENT_TCP
  coap_client_tcp.c
)

zephyr_library_sources_ifdef(CONFIG_COAP_OSCORE
  coap_oscore.c
)
