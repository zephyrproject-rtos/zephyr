/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "esp.h"

/* esp_data->mtx_sock should be held */
struct esp_socket *esp_socket_get(struct esp_data *data)
{
	struct esp_socket *sock;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->sockets); ++i) {
		sock = &data->sockets[i];
		if (!esp_socket_in_use(sock)) {
			break;
		}
	}

	if (esp_socket_in_use(sock)) {
		return NULL;
	}

	sock->link_id = i;
	sock->flags |= ESP_SOCK_IN_USE;

	return sock;
}

/* esp_data->mtx_sock should be held */
int esp_socket_put(struct esp_socket *sock)
{
	sock->flags = 0;
	sock->link_id = INVALID_LINK_ID;
	return 0;
}

struct esp_socket *esp_socket_from_link_id(struct esp_data *data,
					   uint8_t link_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(data->sockets); ++i) {
		if (esp_socket_in_use(&data->sockets[i]) &&
		    data->sockets[i].link_id == link_id) {
			return &data->sockets[i];
		}
	}

	return NULL;
}

void esp_socket_init(struct esp_data *data)
{
	struct esp_socket *sock;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->sockets); ++i) {
		sock = &data->sockets[i];
		sock->idx = i;
		sock->link_id = INVALID_LINK_ID;
		sock->flags = 0;
		k_sem_init(&sock->sem_data_ready, 0, 1);
		k_fifo_init(&sock->fifo_rx_pkt);
	}
}
