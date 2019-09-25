/** @file
 * @brief Modem socket header file.
 *
 * Generic modem socket and packet size implementation for modem context
 */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SOCKET_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SOCKET_H_

#include <kernel.h>
#include <net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

struct modem_socket {
	sa_family_t family;
	enum net_sock_type type;
	enum net_ip_protocol ip_proto;
	struct sockaddr src;
	struct sockaddr dst;
	int id;
	int sock_fd;

	/** packet data */
	u16_t packet_sizes[CONFIG_MODEM_SOCKET_PACKET_COUNT];
	u16_t packet_count;

	/** data ready semaphore */
	struct k_sem sem_data_ready;

	/** socket state */
	bool is_polled;

	/** temporary socket data */
	void *data;
};

struct modem_socket_config {
	struct modem_socket *sockets;
	size_t sockets_len;

	/* beginning socket id (modems can set this to 0 or 1 as needed) */
	int base_socket_num;
	struct k_sem sem_poll;
};

/* return total size of remaining packets */
int modem_socket_packet_size_update(struct modem_socket_config *cfg,
				    struct modem_socket *sock, int new_total);
int modem_socket_get(struct modem_socket_config *cfg, int family, int type,
		     int proto);
struct modem_socket *modem_socket_from_fd(struct modem_socket_config *cfg,
					  int sock_fd);
struct modem_socket *modem_socket_from_id(struct modem_socket_config *cfg,
					  int id);
struct modem_socket *modem_socket_from_newid(struct modem_socket_config *cfg);
void modem_socket_put(struct modem_socket_config *cfg, int sock_fd);
int modem_socket_poll(struct modem_socket_config *cfg,
		      struct pollfd *fds, int nfds, int msecs);
int modem_socket_init(struct modem_socket_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SOCKET_H_ */
