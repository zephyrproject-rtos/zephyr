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

zephyr_sources_ifdef(CONFIG_COAP_OSCORE
  coap_oscore.c
)

zephyr_sources_ifdef(CONFIG_COAP_EDHOC
  coap_edhoc.c
)

zephyr_sources_ifdef(CONFIG_COAP_EDHOC_COMBINED_REQUEST
  coap_oscore_option.c
  coap_edhoc_session.c
  coap_oscore_ctx_cache.c
  coap_edhoc_wrappers.c
  coap_edhoc_combined_blockwise.c
)

zephyr_sources_ifdef(CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC
  coap_edhoc_transport.c
)
