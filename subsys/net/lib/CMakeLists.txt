# SPDX-License-Identifier: Apache-2.0

add_subdirectory(utils)
add_subdirectory_ifdef(CONFIG_COAP coap)
add_subdirectory_ifdef(CONFIG_LWM2M lwm2m)
add_subdirectory_ifdef(CONFIG_SOCKS socks)
add_subdirectory_ifdef(CONFIG_SNTP sntp)
add_subdirectory_ifdef(CONFIG_MQTT_LIB               mqtt)
add_subdirectory_ifdef(CONFIG_MQTT_SN_LIB            mqtt_sn)
add_subdirectory_ifdef(CONFIG_PTP                    ptp)
add_subdirectory_ifdef(CONFIG_TFTP_LIB               tftp)
add_subdirectory_ifdef(CONFIG_NET_CONFIG_SETTINGS    config)
add_subdirectory_ifdef(CONFIG_NET_SOCKETS            sockets)
add_subdirectory_ifdef(CONFIG_TLS_CREDENTIALS        tls_credentials)
add_subdirectory_ifdef(CONFIG_NET_CAPTURE            capture)
add_subdirectory_ifdef(CONFIG_NET_ZPERF              zperf)
add_subdirectory_ifdef(CONFIG_NET_SHELL              shell)
add_subdirectory_ifdef(CONFIG_NET_TRICKLE            trickle)
add_subdirectory_ifdef(CONFIG_NET_DHCPV6             dhcpv6)
add_subdirectory_ifdef(CONFIG_PROMETHEUS             prometheus)

if (CONFIG_NET_DHCPV4 OR CONFIG_NET_DHCPV4_SERVER)
  add_subdirectory(dhcpv4)
endif()

if (CONFIG_DNS_RESOLVER
    OR CONFIG_MDNS_RESPONDER
    OR CONFIG_LLMNR_RESPONDER)
  add_subdirectory(dns)
endif()

if(CONFIG_HTTP)
  add_subdirectory(http)
endif()

add_subdirectory_ifdef(CONFIG_WEBSOCKET_CLIENT websocket)
