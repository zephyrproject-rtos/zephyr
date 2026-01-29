# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_COAP_SERVER_OSCORE)
zephyr_include_directories(${ZEPHYR_UOSCORE_UEDHOC_MODULE_DIR}/test_vectors)
endif()

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

zephyr_sources_ifdef(CONFIG_COAP_SERVER_OSCORE
  edhoc.c
)