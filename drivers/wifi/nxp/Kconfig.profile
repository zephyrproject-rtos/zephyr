# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Part groups (hidden helpers to keep the per-chip defaults readable).
config NXP_WIFI_PART_SLIM
	bool
	default y if NXP_IW610 && NXP_WIFI_SLIM

config NXP_WIFI_PART_HOSTED
	bool
	default y if NXP_IW61X || NXP_IW416 || NXP_88W8987 || NXP_88W8801

# System configs

configdefault THREAD_CUSTOM_DATA
	default y

configdefault REQUIRES_FULL_LIBC
	default y

configdefault CBPRINTF_FP_SUPPORT
	default y

# NXP Wi-Fi networking defaults

choice NET_TC_THREAD_TYPE
	default NET_TC_THREAD_PREEMPTIVE
endchoice

configdefault NET_PKT_RX_COUNT
	default 16 if NXP_WIFI_PART_SLIM
	default 36 if NXP_RW610 || NXP_IW610
	default 72 if NXP_WIFI_PART_HOSTED

configdefault NET_PKT_TX_COUNT
	default 16 if NXP_WIFI_PART_SLIM
	default 36 if NXP_RW610 || NXP_IW610 || NXP_WIFI_PART_HOSTED

configdefault NET_BUF_RX_COUNT
	default 20 if NXP_WIFI_PART_SLIM
	default 40 if NXP_RW610 || NXP_IW610
	default 80 if NXP_WIFI_PART_HOSTED

configdefault NET_BUF_TX_COUNT
	default 20 if NXP_WIFI_PART_SLIM
	default 40 if NXP_RW610 || NXP_IW610 || NXP_WIFI_PART_HOSTED

configdefault NET_MGMT_EVENT_QUEUE_SIZE
	default 20 if NXP_RW610
	default 40 if NXP_IW610 || NXP_WIFI_PART_HOSTED

configdefault NET_MGMT_EVENT_STACK_SIZE
	default 6144 if WIFI_NM_WPA_SUPPLICANT
	default 4608

configdefault NET_MGMT_THREAD_PRIO_CUSTOM
	default y

configdefault NET_MGMT_THREAD_PRIORITY
	default 3

configdefault NET_TCP_WORKQ_STACK_SIZE
	default 2048

configdefault NET_TC_TX_COUNT
	default 1

configdefault NET_TC_RX_COUNT
	default 1

configdefault NET_MAX_CONN
	default 10

configdefault NET_TC_THREAD_PRIO_CUSTOM
	default y

configdefault NET_TC_TX_THREAD_BASE_PRIO
	default 3

configdefault NET_TC_RX_THREAD_BASE_PRIO
	default 3

configdefault NET_TC_TX_SKIP_FOR_HIGH_PRIO
	default y

configdefault NET_CONTEXT_PRIORITY
	default y

configdefault NET_SOCKETS_SERVICE_STACK_SIZE
	default 4096

configdefault NET_SOCKETS_SERVICE_THREAD_PRIO
	default 3

configdefault NET_BUF_LOG
	default y

if NET_TCP

configdefault NET_TCP_MAX_SEND_WINDOW_SIZE
	default 20440 if NXP_WIFI_PART_SLIM
	default 46720 if NXP_RW610 || NXP_IW610 || NXP_WIFI_PART_HOSTED

configdefault NET_TCP_MAX_RECV_WINDOW_SIZE
	default 20440 if NXP_WIFI_PART_SLIM
	default 46720 if NXP_RW610 || NXP_IW610 || NXP_WIFI_PART_HOSTED

configdefault NET_TCP_WORKER_PRIO
	default -16

endif # NET_TCP

if NET_IPV4

configdefault NET_IF_MAX_IPV4_COUNT
	default 2

if NET_IPV4_FRAGMENT

configdefault NET_IPV4_FRAGMENT_MAX_COUNT
	default 3

configdefault NET_IPV4_FRAGMENT_MAX_PKT
	default 7

configdefault NET_IPV4_FRAGMENT_TIMEOUT
	default 3

endif # NET_IPV4_FRAGMENT

endif # NET_IPV4

if NET_IPV6

configdefault NET_IF_MAX_IPV6_COUNT
	default 2

if NET_IPV6_FRAGMENT

configdefault NET_IPV6_FRAGMENT_MAX_COUNT
	default 3

configdefault NET_IPV6_FRAGMENT_MAX_PKT
	default 8

configdefault NET_IPV6_FRAGMENT_TIMEOUT
	default 3

endif # NET_IPV6_FRAGMENT

endif # NET_IPV6

if DNS_RESOLVER

configdefault DNS_RESOLVER_MAX_SERVERS
	default 2

endif # DNS_RESOLVER

if NET_DHCPV4_SERVER

configdefault NET_DHCPV4_SERVER_ADDR_COUNT
	default 32

configdefault NET_DHCPV4_SERVER_ICMP_PROBE_TIMEOUT
	default 100

endif # NET_DHCPV4_SERVER

if NET_ZPERF

configdefault ZPERF_WORK_Q_THREAD_PRIORITY
	default 3

configdefault NET_ZPERF_MAX_SESSIONS
	default 6

configdefault NET_ZPERF_MAX_PACKET_SIZE
	default 1500

endif # NET_ZPERF

if NET_L2_WIFI_MGMT

configdefault WIFI_MGMT_AP_MAX_NUM_STA
	default 8 if NXP_WIFI_SOFTAP_SUPPORT

configdefault WIFI_SHELL_MAX_AP_STA
	default 8

endif # NET_L2_WIFI_MGMT

if NET_STATISTICS

configdefault NET_STATISTICS_WIFI
	default y

configdefault NET_STATISTICS_USER_API
	default y

endif # NET_STATISTICS

configdefault MAIN_STACK_SIZE
	default 4096

configdefault IDLE_STACK_SIZE
	default 1024

configdefault SHELL_STACK_SIZE
	default 6144

configdefault SHELL_ARGC_MAX
	default 48

configdefault SHELL_CMD_BUFF_SIZE
	default 512

configdefault ZVFS_OPEN_MAX
	default 30

configdefault ZVFS_POLL_MAX
	default 14

configdefault LOG_PRINTK
	default n

configdefault CODE_DATA_RELOCATION_SRAM
	default y if NXP_RW610 || NXP_IW610 || NXP_WIFI_PART_HOSTED

configdefault NXP_WIFI_TX_RX_ZERO_COPY
	default y

configdefault NXP_WIFI_FW_DEBUG
	default y if !NXP_WIFI_PART_SLIM

configdefault NXP_WIFI_WAKE_TIMER_ENABLE
	default y if !NXP_WIFI_PART_SLIM

if WIFI_NM_WPA_SUPPLICANT

configdefault MBEDTLS_HEAP_SIZE
	default 75072 if NXP_RW610
	default 90432 if NXP_IW610 || NXP_WIFI_PART_HOSTED

configdefault MBEDTLS_SSL_IN_CONTENT_LEN
	default 8192

configdefault MBEDTLS_SSL_OUT_CONTENT_LEN
	default 8192

configdefault WIFI_NM_MAX_MANAGED_INTERFACES
	default 2

configdefault WIFI_NM_WPA_SUPPLICANT_CLEANUP_INTERVAL
	default 120

if WIFI_NM_HOSTAPD_AP

configdefault WIFI_NM_HOSTAPD_CLEANUP_INTERVAL
	default 120

endif # WIFI_NM_HOSTAPD_AP

configdefault WIFI_NM_WPA_SUPPLICANT_WQ_PRIO
	default 3

configdefault WIFI_NM_WPA_SUPPLICANT_PRIO
	default 3

configdefault WIFI_NM_WPA_SUPPLICANT_WQ_STACK_SIZE
	default 12288

configdefault WIFI_NM_WPA_SUPPLICANT_THREAD_STACK_SIZE
	default 12288

configdefault SAE_PWE_EARLY_EXIT
	default y

configdefault WIFI_NM_WPA_SUPPLICANT_NAN
	default y if NXP_RW610

endif # WIFI_NM_WPA_SUPPLICANT
