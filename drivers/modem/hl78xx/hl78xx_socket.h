/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HL78XX_SOCKET_H
#define HL78XX_SOCKET_H

#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include "tls_internal.h"
#include <zephyr/net/tls_credentials.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "hl78xx.h"
#include "mqtt_offload/mqtt_offload_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Helper macros and constants */
#define MODEM_STREAM_STARTER_WORD "\r\n" CONNECT_STRING "\r\n"
#define MODEM_STREAM_END_WORD     "\r\n" OK_STRING "\r\n"

#define MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT (0)

#define DNS_SERVERS_COUNT                                                                          \
	(0 + (IS_ENABLED(CONFIG_NET_IPV6) ? 1 : 0) + (IS_ENABLED(CONFIG_NET_IPV4) ? 1 : 0) +       \
	 1 /* for NULL terminator */                                                               \
	)
#define L4_EVENT_MASK         (NET_EVENT_DNS_SERVER_ADD | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

#define HL78XX_UART_PIPE_WORK_SOCKET_BUFFER_SIZE 32
#define HL78XX_MAC_ADDR_SIZE                     6

enum hl78xx_tcp_stat_status_code {
	/** Socket not defined, use +KTCPCFG to create a TCP socket */
	TCP_STATUS_UNDEFINED = 0,
	/** Socket is only defined but not used */
	TCP_STATUS_DEFINED = 1,
	/** Socket is opening and connecting to the server, cannot be used */
	TCP_STATUS_CONNECTING = 2,
	/** Connection is up, socket can be used to send/receive data */
	TCP_STATUS_CONNECTED = 3,
	/** Connection is closing, it cannot be used, wait for status 5 */
	TCP_STATUS_CLOSING = 4,
	/** Socket is closed */
	TCP_STATUS_CLOSED = 5
};

enum hl78xx_tcp_socket_status_code {
	/** Connection is up, socket can be used to send/receive data */
	TCP_SOCKET_CONNECTED = 1,
};

enum hl78xx_udp_socket_status_code {
	/** Connection is up, socket can be used to send/receive data */
	UDP_SOCKET_CONNECTED = 1,
};
struct hl78xx_tcp_status {
	enum hl78xx_tcp_socket_status_code err_code;
	bool is_connected;
};
struct hl78xx_udp_status {
	enum hl78xx_udp_socket_status_code err_code;
	bool is_connected;
};

struct mqtt_send_ctx {
	struct modem_socket *sock;
	const char *buf;
	size_t buf_len;
	char *cmd_buf;
	size_t cmd_buf_len;
	char *payload;
	uint16_t *size_payload;
	int *sock_written;
};

/* ---------------- Global Data Structures ---------------- */
struct hl78xx_dns_info {
	char v4_string[NET_IPV4_ADDR_LEN];
	char v6_string[NET_IPV6_ADDR_LEN];
	bool ready;
	struct in_addr v4;
	struct in6_addr v6;
};

struct hl78xx_ipv4_info {
	struct in_addr addr;
	struct in_addr subnet;
	struct in_addr gateway;
	struct in_addr new_addr;
};

struct hl78xx_tls_info {
	char hostname[MDM_MAX_HOSTNAME_LEN];
	bool hostname_set;
};

union hl78xx_mqtt_stat_info {
	struct hl78xx_mqtt_status conn;
	struct hl78xx_mqtt_status subscribe;
	struct hl78xx_mqtt_status unsubscribe;
	struct hl78xx_mqtt_status publish;
};

struct hl78xx_socket_data {
	struct net_if *net_iface;
	uint8_t mac_addr[HL78XX_MAC_ADDR_SIZE];

	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];
	int current_sock_fd;
	int sizeof_socket_data;
	int requested_socket_id;
	int socketopt;

	struct hl78xx_dns_info dns;
	struct hl78xx_ipv4_info ipv4;

	struct ring_buf *buf_pool;
	uint32_t expected_buf_len;
	uint32_t collected_buf_len;

	struct hl78xx_data *mdata_global;

	struct net_mgmt_event_callback l4_cb;
	struct net_mgmt_event_callback conn_cb;

	struct k_sem psm_cntrl_sem;

	bool socket_data_error;

	struct hl78xx_tls_info tls;
	char mqtt_client_id[MDM_MQTT_MAX_CLIENT_ID_LEN];
	struct hl78xx_mqtt_config mqtt_config;
	struct hl78xx_tcp_status tcp_conn_status;
	struct hl78xx_udp_status udp_conn_status;
	union hl78xx_mqtt_stat_info mqtt_status;

	struct k_work_delayable hl78xx_mqtt_work;
};

struct work_socket_data {
	char buf[HL78XX_UART_PIPE_WORK_SOCKET_BUFFER_SIZE];
	uint16_t len;
};

struct receive_socket_data {
	char buf[MDM_MAX_DATA_LENGTH + ARRAY_SIZE(MODEM_STREAM_STARTER_WORD) +
		 ARRAY_SIZE(MODEM_STREAM_END_WORD)];
	uint16_t len;
};

extern struct hl78xx_socket_data socket_data;

void hl78xx_mqtt_work_handler(struct k_work *work_item);
int mqtt_socket_rcv_data_process(const struct modem_socket *sock, const char *data, size_t data_len,
				 char *cmd_buf, size_t cmd_buf_size, int flags);
int mqtt_socket_send_data_process(const struct modem_socket *sock, const char *data,
				  size_t data_len, char *cmd_buf, size_t cmd_buf_size, int flags);

#ifdef __cplusplus
}
#endif

#endif /* HL78XX_SOCKET_H */
