/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Input sections placed in IRAM by CONFIG_ESP32_WIFI_NET_IRAM_OPT. Included
 * in .iram0.text through zephyr_linker_sources(RAMFUNC_SECTION ...).
 */
    /* Per-packet Wi-Fi, IPv4/TCP and socket data path. Each rule is
     * bracketed by symbols so the link can check that it matched.
     */
    _net_iram_esp_wifi_drv_start = ABSOLUTE(.);
    *libdrivers__wifi.a:esp_wifi_drv.*(.literal .literal.* .text .text.*)
    _net_iram_esp_wifi_drv_end = ABSOLUTE(.);
    _net_iram_esp_adapter_start = ABSOLUTE(.);
    *libzephyr.a:esp_adapter.*(.literal .literal.* .text .text.*)
    _net_iram_esp_adapter_end = ABSOLUTE(.);
    _net_iram_net_buf_start = ABSOLUTE(.);
    *liblib__net_buf.a:buf.*(.literal .literal.* .text .text.*)
    _net_iram_net_buf_end = ABSOLUTE(.);
    _net_iram_ethernet_start = ABSOLUTE(.);
    *libsubsys__net__l2__ethernet.a:ethernet.*(.literal .literal.* .text .text.*)
    _net_iram_ethernet_end = ABSOLUTE(.);
    _net_iram_arp_start = ABSOLUTE(.);
    *libsubsys__net__l2__ethernet.a:arp.*(.literal .literal.* .text .text.*)
    _net_iram_arp_end = ABSOLUTE(.);
    _net_iram_net_core_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:net_core.*(.literal .literal.* .text .text.*)
    _net_iram_net_core_end = ABSOLUTE(.);
    _net_iram_net_tc_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:net_tc.*(.literal .literal.* .text .text.*)
    _net_iram_net_tc_end = ABSOLUTE(.);
    _net_iram_net_pkt_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:net_pkt.*(.literal .literal.* .text .text.*)
    _net_iram_net_pkt_end = ABSOLUTE(.);
    _net_iram_net_context_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:net_context.*(.literal .literal.* .text .text.*)
    _net_iram_net_context_end = ABSOLUTE(.);
    _net_iram_connection_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:connection.*(.literal .literal.* .text .text.*)
    _net_iram_connection_end = ABSOLUTE(.);
    _net_iram_utils_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:utils.*(.literal .literal.* .text .text.*)
    _net_iram_utils_end = ABSOLUTE(.);
    _net_iram_ipv4_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:ipv4.*(.literal .literal.* .text .text.*)
    _net_iram_ipv4_end = ABSOLUTE(.);
    _net_iram_tcp_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:tcp.*(.literal .literal.* .text .text.*)
    _net_iram_tcp_end = ABSOLUTE(.);
    _net_iram_net_if_start = ABSOLUTE(.);
    *libsubsys__net__ip.a:net_if.*(.literal.net_if_tx .text.net_if_tx
                                    .literal.net_if_try_queue_tx .text.net_if_try_queue_tx
                                    .literal.net_if_try_send_data .text.net_if_try_send_data
                                    .literal.net_if_recv_data .text.net_if_recv_data)
    _net_iram_net_if_end = ABSOLUTE(.);
    _net_iram_sockets_start = ABSOLUTE(.);
    *libsubsys__net.a:sockets.*(.literal .literal.* .text .text.*)
    _net_iram_sockets_end = ABSOLUTE(.);
    _net_iram_sockets_inet_start = ABSOLUTE(.);
    *libsubsys__net.a:sockets_inet.*(.literal .literal.* .text .text.*)
    _net_iram_sockets_inet_end = ABSOLUTE(.);

/* A placement rule that no longer matches (file renamed or moved to another
 * library) would silently leave its code in flash. Fail the link instead.
 */
ASSERT(_net_iram_esp_wifi_drv_end > _net_iram_esp_wifi_drv_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from drivers/wifi/esp32/src/esp_wifi_drv.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
ASSERT(_net_iram_esp_adapter_end > _net_iram_esp_adapter_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from esp_adapter.c (hal_espressif) was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
ASSERT(_net_iram_net_buf_end > _net_iram_net_buf_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from lib/net_buf/buf.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#if defined(CONFIG_NET_L2_ETHERNET)
ASSERT(_net_iram_ethernet_end > _net_iram_ethernet_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/l2/ethernet/ethernet.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#endif
#if defined(CONFIG_NET_ARP)
ASSERT(_net_iram_arp_end > _net_iram_arp_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/l2/ethernet/arp.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#endif
ASSERT(_net_iram_net_core_end > _net_iram_net_core_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/net_core.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
ASSERT(_net_iram_net_tc_end > _net_iram_net_tc_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/net_tc.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
ASSERT(_net_iram_net_pkt_end > _net_iram_net_pkt_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/net_pkt.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#if defined(CONFIG_NET_CONTEXT)
ASSERT(_net_iram_net_context_end > _net_iram_net_context_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/net_context.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#endif
#if defined(CONFIG_NET_IP) || defined(CONFIG_NET_CONNECTION_SOCKETS)
ASSERT(_net_iram_connection_end > _net_iram_connection_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/connection.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#endif
ASSERT(_net_iram_utils_end > _net_iram_utils_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/utils.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#if defined(CONFIG_NET_IPV4)
ASSERT(_net_iram_ipv4_end > _net_iram_ipv4_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/ipv4.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#endif
#if defined(CONFIG_NET_TCP)
ASSERT(_net_iram_tcp_end > _net_iram_tcp_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/tcp.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#endif
ASSERT(_net_iram_net_if_end > _net_iram_net_if_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/ip/net_if.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#if defined(CONFIG_NET_SOCKETS)
ASSERT(_net_iram_sockets_end > _net_iram_sockets_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/lib/sockets/sockets.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#endif
#if defined(CONFIG_NET_SOCKETS) && defined(CONFIG_NET_NATIVE)
ASSERT(_net_iram_sockets_inet_end > _net_iram_sockets_inet_start,
       "CONFIG_ESP32_WIFI_NET_IRAM_OPT: no code from subsys/net/lib/sockets/sockets_inet.c was placed in IRAM, update its rule in drivers/wifi/esp32/esp32_net_iram.ld");
#endif
