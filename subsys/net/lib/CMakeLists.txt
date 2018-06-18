include_relative_ifdef(CONFIG_COAP                coap/CMakeLists.txt)
include_relative_ifdef(CONFIG_LWM2M               lwm2m/CMakeLists.txt)
include_relative_ifdef(CONFIG_SNTP                sntp/CMakeLists.txt)
include_relative_ifdef(CONFIG_DNS_RESOLVER        dns/CMakeLists.txt)
include_relative_ifdef(CONFIG_MQTT_LEGACY_LIB     mqtt/CMakeLists.txt)
include_relative_ifdef(CONFIG_MQTT_LIB            mqtt_sock/CMakeLists.txt)
include_relative_ifdef(CONFIG_NET_APP             app/CMakeLists.txt)
include_relative_ifdef(CONFIG_NET_CONFIG_SETTINGS config/CMakeLists.txt)
include_relative_ifdef(CONFIG_NET_SOCKETS         sockets/CMakeLists.txt)
include_relative_ifdef(CONFIG_TLS_CREDENTIALS     tls_credentials/CMakeLists.txt)
include_relative_ifdef(CONFIG_WEBSOCKET           websocket/CMakeLists.txt)

if(CONFIG_HTTP_PARSER_URL
    OR CONFIG_HTTP_PARSER
    OR CONFIG_HTTP)
  include_relative(http/CMakeLists.txt)
endif()

add_subdirectory_ifdef(CONFIG_OPENTHREAD_PLAT  ${CMAKE_CURRENT_LIST_DIR}/openthread)
