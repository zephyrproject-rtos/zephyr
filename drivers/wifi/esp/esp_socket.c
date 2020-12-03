/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "esp.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(wifi_esp, CONFIG_WIFI_LOG_LEVEL);

#define RX_NET_PKT_ALLOC_TIMEOUT				\
	K_MSEC(CONFIG_WIFI_ESP_RX_NET_PKT_ALLOC_TIMEOUT)

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

static struct net_pkt *esp_prepare_pkt(struct esp_data *dev,
				       struct net_buf *src,
				       size_t offset, size_t len)
{
	struct net_buf *frag;
	struct net_pkt *pkt;
	size_t to_copy;

	pkt = net_pkt_rx_alloc_with_buffer(dev->net_iface, len, AF_UNSPEC,
					   0, RX_NET_PKT_ALLOC_TIMEOUT);
	if (!pkt) {
		return NULL;
	}

	frag = src;

	/* find the right fragment to start copying from */
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	while (frag && len > 0) {
		to_copy = MIN(len, frag->len - offset);
		if (net_pkt_write(pkt, frag->data + offset, to_copy) != 0) {
			net_pkt_unref(pkt);
			return NULL;
		}

		/* to_copy is always <= len */
		len -= to_copy;
		frag = frag->frags;

		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	net_pkt_cursor_init(pkt);

	return pkt;
}

void esp_socket_rx(struct esp_socket *sock, struct net_buf *buf,
		   size_t offset, size_t len)
{
	struct esp_data *data = esp_socket_to_dev(sock);
	struct net_pkt *pkt;

	if ((sock->flags & (ESP_SOCK_CONNECTED | ESP_SOCK_CLOSE_PENDING)) !=
							ESP_SOCK_CONNECTED) {
		LOG_DBG("Received data on closed link %d", sock->link_id);
		return;
	}

	pkt = esp_prepare_pkt(data, buf, offset, len);
	if (!pkt) {
		LOG_ERR("Failed to get net_pkt: len %zu", len);
		if (sock->type == SOCK_STREAM) {
			sock->flags |= ESP_SOCK_CLOSE_PENDING;
		}
		goto submit_work;
	}

	k_fifo_put(&sock->fifo_rx_pkt, pkt);

submit_work:
	k_work_submit_to_queue(&data->workq, &sock->recv_work);
}

void esp_socket_close(struct esp_socket *sock)
{
	struct esp_data *dev = esp_socket_to_dev(sock);
	char cmd_buf[sizeof("AT+CIPCLOSE=0")];
	int ret;

	snprintk(cmd_buf, sizeof(cmd_buf), "AT+CIPCLOSE=%d",
		 sock->link_id);
	ret = esp_cmd_send(dev, NULL, 0, cmd_buf, ESP_CMD_TIMEOUT);
	if (ret < 0) {
		/* FIXME:
		 * If link doesn't close correctly here, esp_get could
		 * allocate a socket with an already open link.
		 */
		LOG_ERR("Failed to close link %d, ret %d",
			sock->link_id, ret);
	}
}
