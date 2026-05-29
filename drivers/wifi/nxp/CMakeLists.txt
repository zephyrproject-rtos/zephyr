#Copyright 2022 - 2026 NXP
#SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_NXP)

zephyr_library_sources_ifdef(CONFIG_NXP_WIFI_AUTO_INIT src/nxp_wifi_drv.c)

zephyr_library_sources_ifdef(CONFIG_NXP_WIFI_SHELL src/nxp_wifi_shell.c)

zephyr_library_sources(src/nxp_wifi_net.c)

zephyr_include_directories(incl)

zephyr_library_link_libraries_ifdef(CONFIG_WIFI_NM_WPA_SUPPLICANT hostap)

if(CONFIG_CODE_DATA_RELOCATION)
if(CONFIG_SPEED_OPTIMIZATIONS OR CONFIG_SIZE_OPTIMIZATIONS)

if(CONFIG_NXP_RW610)
    set(QUICK_ACCESS_CODE_AREA RAM_TEXT)
    set(QUICK_ACCESS_CODE_AREA_2 RAM_TEXT)
else()
    set(QUICK_ACCESS_CODE_AREA ITCM_TEXT)
    set(QUICK_ACCESS_CODE_AREA_2 DTCM_TEXT)
endif()

string(JOIN "" NXP_PORT_NET_FILTER
       ".*\\.deliver_packet_above|"
       ".*\\.gen_pkt_from_data|"
       ".*\\.gen_tx_pkt_from_data|"
       ".*\\.gen_pkt_from_data_for_zerocopy|"
       ".*\\.process_data_packet|"
       ".*\\.handle_data_packet|"
       ".*\\.handle_amsdu_data_packet|"
       ".*\\.handle_deliver_packet_above|"
       ".*\\.nxp_wifi_internal_tx|"
       ".*\\.net_wifi_packet_send|"
       ".*\\.net_stack_buffer_copy_partial|"
       ".*\\.net_tx_zerocopy_process_cb|"
       ".*\\.net_stack_buffer_alloc_rx|"
       ".*\\.net_stack_buffer_clone_tx_frag|"
       ".*\\.net_stack_buffer_push|"
       ".*\\.net_stack_buffer_set_iface"
)

zephyr_code_relocate(FILES src/nxp_wifi_net.c
                     FILTER "${NXP_PORT_NET_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

zephyr_code_relocate(FILES src/nxp_wifi_drv.c
                     FILTER ".*\\.nxp_wifi_send|.*\\.nxp_wifi_recv"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

file(GLOB ZPERF_SRC ${ZEPHYR_BASE}/subsys/net/lib/zperf/*.c)
zephyr_code_relocate(FILES ${ZPERF_SRC} LOCATION ${QUICK_ACCESS_CODE_AREA_2} NOKEEP)

zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/subsys/net/ip/ipv6_fragment.c
                     ${ZEPHYR_BASE}/subsys/net/ip/ipv4_fragment.c
                     LOCATION RAM_TEXT NOKEEP)

zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/subsys/net/ip/connection.c
                     ${ZEPHYR_BASE}/subsys/net/ip/packet_socket.c
                     ${ZEPHYR_BASE}/subsys/net/ip/utils.c
                     ${ZEPHYR_BASE}/subsys/net/lib/sockets/sockets_inet.c
                     ${ZEPHYR_BASE}/subsys/net/lib/sockets/sockets.c
                     ${ZEPHYR_BASE}/subsys/net/ip/ipv4.c
                     ${ZEPHYR_BASE}/subsys/net/ip/ipv6.c
                     ${ZEPHYR_BASE}/subsys/net/ip/net_context.c
                     ${ZEPHYR_BASE}/subsys/net/ip/net_core.c
                     ${ZEPHYR_BASE}/subsys/net/ip/net_if.c
                     ${ZEPHYR_BASE}/subsys/net/ip/net_pkt.c
                     ${ZEPHYR_BASE}/subsys/net/ip/net_tc.c
                     ${ZEPHYR_BASE}/subsys/net/ip/tcp.c
                     ${ZEPHYR_BASE}/subsys/net/ip/udp.c
                     ${ZEPHYR_BASE}/subsys/net/l2/ethernet/ethernet.c
                     ${ZEPHYR_BASE}/lib/net_buf/buf.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/kernel/mem_slab.c
                     ${ZEPHYR_BASE}/kernel/mempool.c
                     ${ZEPHYR_BASE}/kernel/msg_q.c
                     ${ZEPHYR_BASE}/kernel/mutex.c
                     ${ZEPHYR_BASE}/kernel/queue.c
                     ${ZEPHYR_BASE}/kernel/sched.c
                     ${ZEPHYR_BASE}/kernel/sem.c
                     ${ZEPHYR_BASE}/kernel/thread.c
                     ${ZEPHYR_BASE}/kernel/work.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/lib/net_buf/buf_simple.c
                     ${ZEPHYR_BASE}/subsys/net/lib/sockets/sockets_service.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA_2} NOKEEP)

if(CONFIG_SDIO_STACK)

zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/subsys/sd/sdio.c
                     ${ZEPHYR_BASE}/drivers/sdhc/imx_usdhc.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

endif()
endif()
endif()

endif()
