zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_COAP_NET_PKT
  coap.c
  coap_link_format.c
)
zephyr_sources_ifdef(CONFIG_COAP_SOCK
  coap_sock.c
  coap_link_format_sock.c
)
