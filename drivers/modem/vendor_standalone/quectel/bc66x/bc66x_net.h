/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Giovanni Piccari <giopiccari@outlook.com>
 * SPDX-FileCopyrightText: Copyright 2026 M31 srl <info@m31.com>
 */

#ifndef BC66X_NET_H_
#define BC66X_NET_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/modem/chat.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/net/icmp.h>

#include "bc66x.h"

#define BC66X_SOCKET_RECV_SLAB_COUNT 4

#define BC66X_SOCKET_CMD_BUFFER_SIZE ((BC66X_MAX_HEX_SEND_LENGTH * 2U) + 64U)

#define BC66X_SOCKET_DEFAULT_RECV_TIMEOUT      K_SECONDS(70)
#define BC66X_SOCKET_DEFAULT_SEND_TIMEOUT      K_SECONDS(20)
#define BC66X_SOCKET_DEFAULT_WAIT_LOCK_TIMEOUT K_FOREVER

#define BC66X_SOCKET_SEM_LIMIT 1

#define BC66X_OPEN_SOCKET_AT_COMMAND "AT+QIOPEN=%u,%u,\"%s\",\"%s\",%u,0,1"
#define BC66_SEND_HEX_AT_COMMAND     "AT+QISENDEX=%u,%u,\"%s\""
#define BC660K_SEND_HEX_AT_COMMAND   "AT+QISEND=%u,%u,\"%s\""

struct bc66x_recv_buf {
	void *fifo_reserved; /* must be first for k_fifo */
	uint8_t data[BC66X_SOCKET_RECV_SLAB_SIZE];
	size_t len;
	struct net_sockaddr_in src_addr;
};

enum bc66x_socket_state {
	BC66X_SOCKET_FREE = 0,
	BC66X_SOCKET_ALLOCATED,
	BC66X_SOCKET_OPENING,
	BC66X_SOCKET_OPENED,
	BC66X_SOCKET_CONNECTED,
	BC66X_SOCKET_CLOSING,
	BC66X_SOCKET_CLOSED,
};

struct bc66x_socket {
	struct bc66x_data *parent;

	/* To avoid race conditions. Protects everything except atomic vars. */
	struct k_sem lock;

	uint8_t modem_id;
	atomic_t state; /* enum bc66x_socket_state */

	int type;
	int ip_proto;
	int family;
	struct net_sockaddr_in remote_addr;
	struct net_sockaddr_in local_addr;

	struct k_fifo recv_fifo;
	atomic_t recv_buff_size;

	int sock_fd;

	struct k_sem open_sem;
	int open_result;

	k_timeout_t recv_timeout;
	k_timeout_t send_timeout;

	char cmd_buf[BC66X_SOCKET_CMD_BUFFER_SIZE];

	/* To avoid locking to check */
	atomic_t is_connected;
	atomic_t is_nonblocking;
};

struct bc66x_network_data {
	const struct device *dev;
	struct net_if *iface;

	struct net_in_addr ipv4_addr;

	/* filled at compile time by the device macro per instance  */
	int (*socket_creator)(int family, int type, int proto);

	struct bc66x_socket sockets[BC66X_MAX_SOCKETS];
	struct k_sem sockets_lock; /* Protects the 'sockets' array during alloc/free  */

	/* To handle "SEND OK" / "SEND FAIL".
	 * This cannot be inside socket (no ID specified when
	 * SEND OK is received)
	 */
	struct k_sem send_lock;
	struct k_sem send_sem;
	int send_result;

	/* to handle "CLOSE OK". ("CLOSE FAIL" doesn't exist) */
	struct k_sem close_lock;
	struct k_sem close_sem;

	/* DNS */
	struct k_sem dns_lock; /* To have only one DNS request in progress */
	struct k_sem dns_sem;  /* Signals when the response is received */
	int16_t dns_error;
	char dns_host_ip[NET_IPV4_ADDR_LEN]; /* Response of DNS request */
	bool dns_wait_ip; /* Used to discriminate between the two response formats */

	/* PING */
	struct net_icmp_offload icmp_offload;
	struct net_icmp_ctx *active_ping_ctx;
	void *active_ping_user_data;
};

/* URC handlers */
void on_urc_qiopen(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void on_urc_closed(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void on_urc_recv(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void on_qidnsgip(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void on_urc_qiping(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void on_urc_dnsgip(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);

/* Response handles (Not URCs) */
void on_send_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void on_send_fail(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);
void on_close_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data);

/* Driver init (network "side") */
int bc66x_network_init(const struct device *dev);
void bc66x_iface_init(struct net_if *iface);

bool bc66x_offload_is_supported(int family, int type, int proto);
int bc66x_create_socket(struct bc66x_data *data, int family, int type, int proto);

#endif /* BC66X_NET_H_ */
