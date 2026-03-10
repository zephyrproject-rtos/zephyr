#
# Copyright (c) 2025 Netfeasa Ltd.
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(
    hl78xx.c
    hl78xx_sockets.c
    hl78xx_cfg.c
    hl78xx_chat.c
    hl78xx_apis.c
)

zephyr_library_sources_ifdef(
    CONFIG_MODEM_HL78XX_AT_SHELL
    hl78xx_at_shell.c
)

zephyr_library_sources_ifdef(
    CONFIG_HL78XX_GNSS
    hl78xx_gnss.c
    hl78xx_gnss_parsers.c
)

add_subdirectory_ifdef(CONFIG_HL78XX_EVT_MONITOR hl78xx_evt_monitor)

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

if(CONFIG_HL78XX_GNSS_AUX_DATA_PARSER)
  zephyr_iterable_section(NAME hl78xx_gnss_data_callback KVMA RAM_REGION GROUP RODATA_REGION)
  zephyr_linker_sources(SECTIONS gnss_linker.ld)
endif()
