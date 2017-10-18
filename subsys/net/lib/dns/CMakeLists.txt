zephyr_include_directories(.)

zephyr_sources(
  dns_pack.c
)
zephyr_sources_ifdef(CONFIG_DNS_RESOLVER  resolve.c)
zephyr_sources_ifdef(CONFIG_MDNS_RESOLVER mdns_responder.c)
