# SPDX-License-Identifier: Apache-2.0

add_subdirectory(utils)
add_subdirectory_ifdef(CONFIG_COAP coap)
add_subdirectory_ifdef(CONFIG_LWM2M lwm2m)
add_subdirectory_ifdef(CONFIG_SOCKS socks)
add_subdirectory_ifdef(CONFIG_SNTP sntp)
add_subdirectory_ifdef(CONFIG_MQTT_LIB               mqtt)
add_subdirectory_ifdef(CONFIG_TFTP_LIB               tftp)
add_subdirectory_ifdef(CONFIG_NET_CONFIG_SETTINGS    config)
add_subdirectory_ifdef(CONFIG_NET_SOCKETS            sockets)
add_subdirectory_ifdef(CONFIG_TLS_CREDENTIALS        tls_credentials)
add_subdirectory_ifdef(CONFIG_NET_CONNECTION_MANAGER conn_mgr)
add_subdirectory_ifdef(CONFIG_NET_CAPTURE            capture)

if (CONFIG_DNS_RESOLVER
    OR CONFIG_MDNS_RESPONDER
    OR CONFIG_LLMNR_RESPONDER)
  add_subdirectory(dns)
endif()

if(CONFIG_HTTP_PARSER_URL OR CONFIG_HTTP_PARSER OR CONFIG_HTTP_CLIENT)
  add_subdirectory(http)
endif()

add_subdirectory_ifdef(CONFIG_WEBSOCKET_CLIENT websocket)
add_subdirectory_ifdef(CONFIG_NET_L2_OPENTHREAD openthread)
