zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_COAP
  coap.c
  coap_link_format.c
)
