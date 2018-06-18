zephyr_list(SOURCES
  OUTPUT PRIVATE_SOURCES
  IFDEF:${CONFIG_COAP_NET_PKT} coap.c
  IFDEF:${CONFIG_COAP_NET_PKT} coap_link_format.c
  IFDEF:${CONFIG_COAP_SOCK}    coap_sock.c
  IFDEF:${CONFIG_COAP_SOCK}    coap_link_format_sock
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})
target_include_directories(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR})
