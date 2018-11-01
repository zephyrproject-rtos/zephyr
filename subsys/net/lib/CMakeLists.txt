add_subdirectory_if_kconfig(coap)
add_subdirectory_if_kconfig(lwm2m)
add_subdirectory_if_kconfig(sntp)
add_subdirectory_ifdef(CONFIG_DNS_RESOLVER      dns)
add_subdirectory_ifdef(CONFIG_MQTT_LIB          mqtt)
add_subdirectory_ifdef(CONFIG_NET_APP           app)
add_subdirectory_ifdef(CONFIG_NET_CONFIG_SETTINGS config)
add_subdirectory_ifdef(CONFIG_NET_SOCKETS       sockets)
add_subdirectory_ifdef(CONFIG_TLS_CREDENTIALS   tls_credentials)
add_subdirectory_ifdef(CONFIG_WEBSOCKET         websocket)

if(CONFIG_HTTP_PARSER_URL
    OR CONFIG_HTTP_PARSER
    OR CONFIG_HTTP)
  add_subdirectory(http)
endif()

add_subdirectory_ifdef(CONFIG_OPENTHREAD_PLAT openthread)
