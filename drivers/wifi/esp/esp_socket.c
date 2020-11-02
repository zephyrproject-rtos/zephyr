/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "esp.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(wifi_esp);

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

void esp_socket_close(struct esp_socket *sock)
{
	struct esp_data *dev = esp_socket_to_dev(sock);
	char cmd_buf[16];
	int ret;

	snprintk(cmd_buf, sizeof(cmd_buf), "AT+CIPCLOSE=%d",
		 sock->link_id);
	ret = modem_cmd_send(&dev->mctx.iface, &dev->mctx.cmd_handler,
			     NULL, 0, cmd_buf, &dev->sem_response,
			     ESP_CMD_TIMEOUT);
	if (ret < 0) {
		/* FIXME:
		 * If link doesn't close correctly here, esp_get could
		 * allocate a socket with an already open link.
		 */
		LOG_ERR("Failed to close link %d, ret %d",
			sock->link_id, ret);
	}
}
