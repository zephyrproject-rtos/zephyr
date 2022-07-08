/** @file
 * @brief Modem socket header file.
 *
 * Generic modem socket and packet size implementation for modem context
 */

/*
 * Copyright (c) 2019-2020 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SOCKET_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SOCKET_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#include "sockets_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

__net_socket struct modem_socket {
	sa_family_t family;
	enum net_sock_type type;
	int ip_proto;
	struct sockaddr src;
	struct sockaddr dst;

	/** The number identifying the socket handle inside the modem */
	int id;

	/** The file descriptor identifying the socket in the fdtable */
	int sock_fd;

	/** packet data */
	uint16_t packet_sizes[CONFIG_MODEM_SOCKET_PACKET_COUNT];
	uint16_t packet_count;

	/** data ready semaphore */
	struct k_sem sem_data_ready;
	/** data ready poll signal */
	struct k_poll_signal sig_data_ready;

	/** socket state */
	bool is_connected;
	bool is_waiting;

	/** temporary socket data */
	void *data;
};

struct modem_socket_config {
	struct modem_socket *sockets;
	size_t sockets_len;

	/* beginning socket id (modems can set this to 0 or 1 as needed) */
	int base_socket_num;
	struct k_sem sem_lock;

	const struct socket_op_vtable *vtable;
};

/* return size of the first packet */
uint16_t modem_socket_next_packet_size(struct modem_socket_config *cfg, struct modem_socket *sock);
int modem_socket_packet_size_update(struct modem_socket_config *cfg, struct modem_socket *sock,
				    int new_total);
int modem_socket_get(struct modem_socket_config *cfg, int family, int type, int proto);
struct modem_socket *modem_socket_from_fd(struct modem_socket_config *cfg, int sock_fd);
struct modem_socket *modem_socket_from_id(struct modem_socket_config *cfg, int id);
struct modem_socket *modem_socket_from_newid(struct modem_socket_config *cfg);
void modem_socket_put(struct modem_socket_config *cfg, int sock_fd);
int modem_socket_poll(struct modem_socket_config *cfg, struct zsock_pollfd *fds, int nfds,
		      int msecs);
int modem_socket_poll_update(struct modem_socket *sock, struct zsock_pollfd *pfd,
			     struct k_poll_event **pev);
int modem_socket_poll_prepare(struct modem_socket_config *cfg, struct modem_socket *sock,
			      struct zsock_pollfd *pfd, struct k_poll_event **pev,
			      struct k_poll_event *pev_end);
void modem_socket_wait_data(struct modem_socket_config *cfg, struct modem_socket *sock);
void modem_socket_data_ready(struct modem_socket_config *cfg, struct modem_socket *sock);
int modem_socket_init(struct modem_socket_config *cfg, const struct socket_op_vtable *vtable);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SOCKET_H_ */
