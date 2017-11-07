add_subdirectory_if_kconfig(coap)
add_subdirectory_if_kconfig(lwm2m)
add_subdirectory_if_kconfig(sntp)
add_subdirectory_if_kconfig(zoap)
add_subdirectory_ifdef(CONFIG_DNS_RESOLVER      dns)
add_subdirectory_ifdef(CONFIG_MQTT_LIB          mqtt)
add_subdirectory_ifdef(CONFIG_NET_APP_SETTINGS  app)
add_subdirectory_ifdef(CONFIG_NET_SOCKETS       sockets)

if(CONFIG_HTTP_PARSER_URL
    OR CONFIG_HTTP_PARSER
    OR CONFIG_HTTP)
  add_subdirectory(http)
endif()

