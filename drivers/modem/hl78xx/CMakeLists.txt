#
#  Copyright (c) 2025 Netfeasa Ltd.
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(
    hl78xx.c
    hl78xx_sockets.c
    hl78xx_utility.c
    hl78xx_apis.c
)

zephyr_library_sources_ifdef(
    CONFIG_MODEM_HL78XX_AT_SHELL
    hl78xx_at_shell.c
)

add_subdirectory_ifdef(CONFIG_HL78XX_EVT_MONITOR hl78xx_evt_monitor)
add_subdirectory_ifdef(CONFIG_MQTT_OFFLOAD_LIB mqtt_offload)

zephyr_library_include_directories(
  ./
  # IP headers
  ${ZEPHYR_BASE}/subsys/net/ip
  ${ZEPHYR_BASE}/subsys/net/lib/sockets
)

zephyr_library_include_directories_ifdef(
  CONFIG_NET_SOCKETS_SOCKOPT_TLS
  ${ZEPHYR_BASE}/subsys/net/lib/tls_credentials
)

zephyr_library_include_directories_ifdef(
  CONFIG_MQTT_LIB_CUSTOM_TRANSPORT
  ${ZEPHYR_BASE}/subsys/net/lib/mqtt
)
