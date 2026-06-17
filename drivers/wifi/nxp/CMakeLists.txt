#Copyright 2022 - 2026 NXP
#SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_NXP)

zephyr_library_sources_ifdef(CONFIG_NXP_WIFI_AUTO_INIT src/nxp_wifi_drv.c)

zephyr_library_sources_ifdef(CONFIG_NXP_WIFI_SHELL src/nxp_wifi_shell.c)

zephyr_library_sources(src/nxp_wifi_net.c)

zephyr_include_directories(incl)

zephyr_library_link_libraries_ifdef(CONFIG_WIFI_NM_WPA_SUPPLICANT hostap)

if(CONFIG_CODE_DATA_RELOCATION)
if(CONFIG_SPEED_OPTIMIZATIONS)

if(CONFIG_NXP_RW610)
    set(QUICK_ACCESS_CODE_AREA RAM_TEXT)
    set(QUICK_ACCESS_CODE_AREA_2 RAM_TEXT)
else()
    set(QUICK_ACCESS_CODE_AREA ITCM_TEXT)
    set(QUICK_ACCESS_CODE_AREA_2 DTCM_TEXT)
endif()

# NXP SHIM DRIVER
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

# ZPERF
string(JOIN "" NXP_ZPERF_UDP_UP_FILTER
       ".*\\.udp_upload|"
       ".*\\.cal_compensate_delay"
)
string(JOIN "" NXP_ZPERF_TCP_UP_FILTER
       ".*\\.tcp_upload|"
       ".*\\.sendall"
)
string(JOIN "" NXP_ZPERF_UDP_RX_FILTER
       ".*\\.udp_recv_data|"
       ".*\\.udp_received|"
       ".*\\.udp_svc_handler|"
       ".*\\.build_reply|"
       ".*\\.zperf_receiver_send_stat"
)
string(JOIN "" NXP_ZPERF_TCP_RX_FILTER
       ".*\\.tcp_recv_data|"
       ".*\\.tcp_received|"
       ".*\\.tcp_svc_handler"
)
string(JOIN "" NXP_ZPERF_SESSION_FILTER
       ".*\\.get_session|"
       ".*\\.zperf_reset_session_stats"
)
string(JOIN "" NXP_ZPERF_COMMON_FILTER
       ".*\\.zperf_packet_duration"
)

zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/zperf/zperf_udp_uploader.c
                     FILTER "${NXP_ZPERF_UDP_UP_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/zperf/zperf_tcp_uploader.c
                     FILTER "${NXP_ZPERF_TCP_UP_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/zperf/zperf_udp_receiver.c
                     FILTER "${NXP_ZPERF_UDP_RX_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/zperf/zperf_tcp_receiver.c
                     FILTER "${NXP_ZPERF_TCP_RX_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/zperf/zperf_session.c
                     FILTER "${NXP_ZPERF_SESSION_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/zperf/zperf_common.c
                     FILTER "${NXP_ZPERF_COMMON_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

# NET STACK CORE
zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/subsys/net/ip/net_pkt.c
                     ${ZEPHYR_BASE}/subsys/net/ip/tcp.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_NET_IF_FILTER
       ".*\\.net_if_tx|"
       ".*\\.net_if_send_data|"
       ".*\\.net_if_try_send_data|"
       ".*\\.net_if_try_queue_tx|"
       ".*\\.net_if_queue_tx|"
       ".*\\.net_process_tx_packet|"
       ".*\\.net_if_recv_data|"
       ".*\\.net_if_get_mtu"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/ip/net_if.c
                     FILTER "${NXP_NET_IF_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_NET_CORE_FILTER
       ".*\\.net_recv_data|"
       ".*\\.net_queue_rx|"
       ".*\\.net_process_rx_packet|"
       ".*\\.net_try_send_data|"
       ".*\\.net_send_data|"
       ".*\\.processing_data|"
       ".*\\.net_rx|"
       ".*\\.check_ip"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/ip/net_core.c
                     FILTER "${NXP_NET_CORE_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_NET_TC_FILTER
       ".*\\.net_tc_try_submit_to_tx_queue|"
       ".*\\.net_tc_submit_to_rx_queue|"
       ".*\\.net_tx_priority2tc|"
       ".*\\.net_rx_priority2tc|"
       ".*\\.tc_rx_handler|"
       ".*\\.tc_tx_handler"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/ip/net_tc.c
                     FILTER "${NXP_NET_TC_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_CONNECTION_FILTER
       ".*\\.net_conn_input|"
       ".*\\.net_conn_packet_input|"
       ".*\\.net_conn_raw_ip_input"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/ip/connection.c
                     FILTER "${NXP_CONNECTION_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_IPV4_FILTER
       ".*\\.net_ipv4_create_full|"
       ".*\\.net_ipv4_create|"
       ".*\\.net_ipv4_finalize|"
       ".*\\.net_ipv4_input|"
       ".*\\.net_ipv4_prepare_for_send"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/ip/ipv4.c
                     FILTER "${NXP_IPV4_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_IPV6_FILTER
       ".*\\.net_ipv6_input|"
       ".*\\.net_ipv6_create|"
       ".*\\.net_ipv6_finalize|"
       ".*\\.net_ipv6_handle_fragment_hdr"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/ip/ipv6.c
                     FILTER "${NXP_IPV6_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_NET_CONTEXT_FILTER
       ".*\\.context_sendto|"
       ".*\\.context_setup_udp_packet|"
       ".*\\.context_finalize_packet|"
       ".*\\.context_alloc_pkt|"
       ".*\\.context_write_data|"
       ".*\\.context_validate_dont_fragment_packet|"
       ".*\\.net_context_create_ipv4_new|"
       ".*\\.net_context_create_ipv6_new|"
       ".*\\.net_context_send|"
       ".*\\.net_context_sendto|"
       ".*\\.net_context_packet_received|"
       ".*\\.get_context_priority"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/ip/net_context.c
                     FILTER "${NXP_NET_CONTEXT_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_UTILS_FILTER
       ".*\\.net_calc_chksum|"
       ".*\\.net_calc_chksum_ipv4|"
       ".*\\.net_calc_chksum_udp|"
       ".*\\.net_calc_chksum_tcp|"
       ".*\\.net_calc_verify_chksum_udp"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/ip/utils.c
                     FILTER "${NXP_UTILS_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_ETHERNET_FILTER
       ".*\\.ethernet_recv|"
       ".*\\.ethernet_send|"
       ".*\\.ethernet_fill_header|"
       ".*\\.ethernet_update_tx_stats|"
       ".*\\.ethernet_update_rx_stats|"
       ".*\\.ethernet_update_length|"
       ".*\\.ethernet_ip_recv|"
       ".*\\.ethernet_ll_prepare_on_ipv4|"
       ".*\\.ethernet_fill_in_dst_on_ipv4_mcast|"
       ".*\\.ethernet_fill_in_dst_on_ipv6_mcast"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/l2/ethernet/ethernet.c
                     FILTER "${NXP_ETHERNET_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/subsys/net/ip/packet_socket.c
                     ${ZEPHYR_BASE}/subsys/net/ip/udp.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

# KERNEL
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/mem_slab.c
                     FILTER ".*\\.k_mem_slab_alloc|.*\\.k_mem_slab_free"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_MEMPOOL_FILTER
       ".*\\.k_malloc|"
       ".*\\.k_free|"
       ".*\\.z_thread_malloc"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/mempool.c
                     FILTER "${NXP_MEMPOOL_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_MUTEX_FILTER
       ".*\\.z_impl_k_mutex_lock|"
       ".*\\.z_impl_k_mutex_unlock|"
       ".*\\.new_prio_for_inheritance|"
       ".*\\.adjust_owner_prio"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/mutex.c
                     FILTER "${NXP_MUTEX_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/kernel/sched.c
                     ${ZEPHYR_BASE}/kernel/queue.c
                     ${ZEPHYR_BASE}/kernel/sem.c
                     ${ZEPHYR_BASE}/kernel/work.c
                     ${ZEPHYR_BASE}/kernel/msg_q.c
                     ${ZEPHYR_BASE}/kernel/poll.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_THREAD_FILTER
       ".*\\.k_is_in_isr|"
       ".*\\.z_thread_mark_switched_in|"
       ".*\\.z_thread_mark_switched_out|"
       ".*\\.z_impl_k_is_preempt_thread|"
       ".*\\.z_impl_k_thread_priority_get|"
       ".*\\.z_check_stack_sentinel|"
       ".*\\.z_impl_k_yield"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/thread.c
                     FILTER "${NXP_THREAD_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/sleep.c
                     FILTER ".*\\.z_impl_k_sleep|.*\\.z_tick_sleep"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_CONDVAR_FILTER
       ".*\\.z_impl_k_condvar_signal|"
       ".*\\.z_impl_k_condvar_wait|"
       ".*\\.k_condvar_signal|"
       ".*\\.k_condvar_wait"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/condvar.c
                     FILTER "${NXP_CONDVAR_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_TIMEOUT_FILTER
       ".*\\.z_add_timeout|"
       ".*\\.z_abort_timeout|"
       ".*\\.z_is_inactive_timeout|"
       ".*\\.sys_clock_announce_locked|"
       ".*\\.z_timeout_expires|"
       ".*\\.z_get_next_timeout_expiry"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/kernel/timeout.c
                     FILTER "${NXP_TIMEOUT_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

# SOCKETS LAYER
string(JOIN "" NXP_SOCKETS_FILTER
       ".*\\.get_sock_vtable|"
       ".*\\.z_impl_zsock_sendto|"
       ".*\\.z_impl_zsock_sendmsg|"
       ".*\\.z_impl_zsock_recvfrom|"
       ".*\\.z_impl_zsock_recvmsg|"
       ".*\\.z_impl_zsock_setsockopt|"
       ".*\\.z_impl_zsock_getsockopt|"
       ".*\\.z_impl_zsock_fcntl_impl|"
       ".*\\.z_impl_zsock_ioctl_impl"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/sockets/sockets.c
                     FILTER "${NXP_SOCKETS_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_SOCKETS_INET_FILTER
       ".*\\.zsock_sendto_ctx|"
       ".*\\.send_check_and_wait|"
       ".*\\.zsock_sendmsg_ctx|"
       ".*\\.zsock_recvfrom_ctx|"
       ".*\\.zsock_recvmsg_ctx|"
       ".*\\.zsock_recv_dgram|"
       ".*\\.zsock_recv_stream|"
       ".*\\.zsock_recv_stream_timed|"
       ".*\\.zsock_recv_stream_immediate|"
       ".*\\.zsock_wait_data|"
       ".*\\.zsock_received_cb|"
       ".*\\.sock_get_pkt_src_addr|"
       ".*\\.fifo_wait_non_empty|"
       ".*\\.net_socket_update_tc_rx_time|"
       ".*\\.sock_get_stream_src_addr|"
       ".*\\.zsock_poll_prepare_ctx|"
       ".*\\.zsock_poll_update_ctx|"
       ".*\\.zsock_fionread_ctx|"
       ".*\\.zsock_fionwrite_ctx"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/sockets/sockets_inet.c
                     FILTER "${NXP_SOCKETS_INET_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_SOCKETS_SVC_FILTER
       ".*\\.socket_service_thread|"
       ".*\\.trigger_work|"
       ".*\\.call_work|"
       ".*\\.find_svc_and_event|"
       ".*\\.net_socket_service_callback|"
       ".*\\.z_impl_net_socket_service_register"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/net/lib/sockets/sockets_service.c
                     FILTER "${NXP_SOCKETS_SVC_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

# ZVFS
zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/lib/os/zvfs/zvfs_poll.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_ZVFS_FDTABLE_FILTER
       ".*\\.zvfs_get_fd_obj|"
       ".*\\.zvfs_get_fd_obj_and_vtable|"
       ".*\\._check_fd|"
       ".*\\.z_fd_ref|"
       ".*\\.z_fd_unref|"
       ".*\\.zvfs_rw|"
       ".*\\.zvfs_read|"
       ".*\\.zvfs_write|"
       ".*\\.zvfs_get_obj_lock_and_cond"
)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/lib/os/zvfs/zvfs_fdtable.c
                     FILTER "${NXP_ZVFS_FDTABLE_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

# NET_BUF
zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/lib/net_buf/buf_simple.c
                     ${ZEPHYR_BASE}/lib/net_buf/buf.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

# CACHE
if(CONFIG_CPU_CORTEX_M AND CONFIG_ARCH_CACHE)

zephyr_code_relocate(FILES
                     ${ZEPHYR_BASE}/arch/arm/core/cortex_m/cache.c
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

endif()

# SDIO
if(CONFIG_SDIO_STACK)

string(JOIN "" NXP_ZEPHYR_SDIO_FILTER
       ".*\\.sdio_io_rw_direct|"
       ".*\\.sdio_io_rw_extended|"
       ".*\\.sdio_io_rw_extended_helper|"
       ".*\\.sdio_read_byte|"
       ".*\\.sdio_write_byte|"
       ".*\\.sdio_rw_byte|"
       ".*\\.sdio_read_fifo|"
       ".*\\.sdio_write_fifo|"
       ".*\\.sdio_read_blocks_fifo|"
       ".*\\.sdio_write_blocks_fifo|"
       ".*\\.sdio_read_addr|"
       ".*\\.sdio_write_addr"
)

zephyr_code_relocate(FILES ${ZEPHYR_BASE}/subsys/sd/sdio.c
                     FILTER "${NXP_ZEPHYR_SDIO_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

string(JOIN "" NXP_DRIVERS_USDHC_FILTER
       ".*\\.transfer_complete_cb|"
       ".*\\.sdio_interrupt_cb|"
       ".*\\.imx_usdhc_dat3_pull|"
       ".*\\.imx_usdhc_dcache_pre_xfer|"
       ".*\\.imx_usdhc_dcache_post_xfer|"
       ".*\\.imx_usdhc_transfer|"
       ".*\\.imx_usdhc_card_busy|"
       ".*\\.imx_usdhc_to_sg_data|"
       ".*\\.imx_usdhc_fill_sg_list|"
       ".*\\.imx_usdhc_request|"
       ".*\\.imx_usdhc_isr"
)

zephyr_code_relocate(FILES ${ZEPHYR_BASE}/drivers/sdhc/imx_usdhc.c
                     FILTER "${NXP_DRIVERS_USDHC_FILTER}"
                     LOCATION ${QUICK_ACCESS_CODE_AREA} NOKEEP)

endif()
endif()
endif()

endif()
