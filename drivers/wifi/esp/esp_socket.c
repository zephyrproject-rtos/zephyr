/*
 * Copyright (c) 2019 Tobias Svehagen
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(wifi_esp, CONFIG_WIFI_LOG_LEVEL);

#define RX_NET_PKT_ALLOC_TIMEOUT				\
	K_MSEC(CONFIG_WIFI_ESP_RX_NET_PKT_ALLOC_TIMEOUT)

struct esp_workq_flush_data {
	struct k_work work;
	struct k_sem sem;
};

struct esp_socket *esp_socket_get(struct esp_data *data,
				  struct net_context *context)
{
	struct esp_socket *sock = data->sockets;
	struct esp_socket *sock_end = sock + ARRAY_SIZE(data->sockets);

	for (; sock < sock_end; sock++) {
		if (!esp_socket_flags_test_and_set(sock, ESP_SOCK_IN_USE)) {
			/* here we should configure all the stuff needed */
			sock->context = context;
			context->offload_context = sock;

			sock->connect_cb = NULL;
			sock->recv_cb = NULL;

			atomic_inc(&sock->refcount);

			return sock;
		}
	}

	return NULL;
}

int esp_socket_put(struct esp_socket *sock)
{
	atomic_clear(&sock->flags);

	return 0;
}

struct esp_socket *esp_socket_ref(struct esp_socket *sock)
{
	atomic_val_t ref;

	do {
		ref = atomic_get(&sock->refcount);
		if (!ref) {
			return NULL;
		}
	} while (!atomic_cas(&sock->refcount, ref, ref + 1));

	return sock;
}

void esp_socket_unref(struct esp_socket *sock)
{
	atomic_val_t ref;

	do {
		ref = atomic_get(&sock->refcount);
		if (!ref) {
			return;
		}
	} while (!atomic_cas(&sock->refcount, ref, ref - 1));

	k_sem_give(&sock->sem_free);
}

void esp_socket_init(struct esp_data *data)
{
	struct esp_socket *sock;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->sockets); ++i) {
		sock = &data->sockets[i];
		sock->idx = i;
		sock->link_id = i;
		atomic_clear(&sock->refcount);
		atomic_clear(&sock->flags);
		k_mutex_init(&sock->lock);
		k_sem_init(&sock->sem_data_ready, 0, 1);
		k_work_init(&sock->connect_work, esp_connect_work);
		k_work_init(&sock->recvdata_work, esp_recvdata_work);
		k_work_init(&sock->close_work, esp_close_work);
	}
}

static struct net_pkt *esp_socket_prepare_pkt(struct esp_socket *sock,
					      struct net_buf *src,
					      size_t offset, size_t len)
{
	struct esp_data *data = esp_socket_to_dev(sock);
	struct net_buf *frag;
	struct net_pkt *pkt;
	size_t to_copy;

	pkt = net_pkt_rx_alloc_with_buffer(data->net_iface, len, AF_UNSPEC,
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

	net_pkt_set_context(pkt, sock->context);
	net_pkt_cursor_init(pkt);

	return pkt;
}

void esp_socket_rx(struct esp_socket *sock, struct net_buf *buf,
		   size_t offset, size_t len)
{
	struct net_pkt *pkt;
	atomic_val_t flags;

	flags = esp_socket_flags(sock);

	if (!(flags & ESP_SOCK_CONNECTED) ||
	    (flags & ESP_SOCK_CLOSE_PENDING)) {
		LOG_DBG("Received data on closed link %d", sock->link_id);
		return;
	}

	pkt = esp_socket_prepare_pkt(sock, buf, offset, len);
	if (!pkt) {
		LOG_ERR("Failed to get net_pkt: len %zu", len);
		if (esp_socket_type(sock) == SOCK_STREAM) {
			if (!esp_socket_flags_test_and_set(sock,
						ESP_SOCK_CLOSE_PENDING)) {
				esp_socket_work_submit(sock, &sock->close_work);
			}
		}
		return;
	}

	k_mutex_lock(&sock->lock, K_FOREVER);
	if (sock->recv_cb) {
		sock->recv_cb(sock->context, pkt, NULL, NULL,
			      0, sock->recv_user_data);
		k_sem_give(&sock->sem_data_ready);
	} else {
		/* Discard */
		net_pkt_unref(pkt);
	}
	k_mutex_unlock(&sock->lock);
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

static void esp_workq_flush_work(struct k_work *work)
{
	struct esp_workq_flush_data *flush =
		CONTAINER_OF(work, struct esp_workq_flush_data, work);

	k_sem_give(&flush->sem);
}

void esp_socket_workq_stop_and_flush(struct esp_socket *sock)
{
	struct esp_workq_flush_data flush;

	k_work_init(&flush.work, esp_workq_flush_work);
	k_sem_init(&flush.sem, 0, 1);

	k_mutex_lock(&sock->lock, K_FOREVER);
	esp_socket_flags_set(sock, ESP_SOCK_WORKQ_STOPPED);
	__esp_socket_work_submit(sock, &flush.work);
	k_mutex_unlock(&sock->lock);

	k_sem_take(&flush.sem, K_FOREVER);
}
