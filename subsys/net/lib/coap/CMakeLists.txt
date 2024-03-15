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

zephyr_sources_ifdef(CONFIG_COAP_SERVER_SHELL
  coap_server_shell.c
)
