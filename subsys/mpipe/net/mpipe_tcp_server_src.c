/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/net/mpipe_tcp_server_src.h>

#include "tcp_server.h"

LOG_MODULE_REGISTER(mpipe_tcp_server_src, CONFIG_MPIPE_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mpipe_tcp_server_src_nb_pool, CONFIG_MPIPE_NET_SRC_NUM_BUFS,
			  CONFIG_MPIPE_NET_SRC_BUF_SIZE, sizeof(struct mpipe_buffer_meta),
			  mpipe_buffer_destroy);

static int mpipe_tcp_server_src_set_property(struct mpipe_object *obj, uint32_t key,
					     const void *val)
{
	struct mpipe_tcp_server_src *tsrc = (struct mpipe_tcp_server_src *)obj;

	switch (key) {
	case MPIPE_PROP_TCP_SERVER_SRC_PORT:
		if (val == NULL) {
			return -EINVAL;
		}
		tsrc->port = *(const uint16_t *)val;
		return 0;
	default:
		return mpipe_src_set_property(obj, key, val);
	}
}

static int mpipe_tcp_server_src_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_tcp_server_src *tsrc = (struct mpipe_tcp_server_src *)obj;

	switch (key) {
	case MPIPE_PROP_TCP_SERVER_SRC_PORT:
		*(uint16_t *)val = tsrc->port;
		return 0;
	default:
		return mpipe_src_get_property(obj, key, val);
	}
}

static int mpipe_tcp_server_src_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **buf)
{
	struct mpipe_tcp_server_src *tsrc = CONTAINER_OF(pool, struct mpipe_tcp_server_src, pool);
	struct mpipe_buffer_meta *meta;
	struct net_buf *nb;
	ssize_t rd;

	if (tsrc->client_fd < 0) {
		return -ENOTCONN;
	}

	nb = net_buf_alloc(pool->nb_pool, K_NO_WAIT);
	if (nb == NULL) {
		return -ENOBUFS;
	}

	meta = mpipe_buffer_get_meta(nb);
	meta->pool = pool;

	rd = zsock_recv(tsrc->client_fd, nb->data, nb->size, 0);
	if (rd <= 0) {
		/* A closed connection is the end of the stream */
		int ret = (rd == 0) ? -ENODATA : -errno;

		if (rd < 0) {
			LOG_ERR("recv() failed (%d)", errno);
		}
		net_buf_unref(nb);
		return ret;
	}

	meta->bytes_used = rd;
	meta->timestamp = k_uptime_get_32();
	nb->len = rd;
	*buf = nb;

	return 0;
}

static int mpipe_tcp_server_src_change_state(struct mpipe_element *self,
					     enum mpipe_state_change transition)
{
	struct mpipe_tcp_server_src *tsrc = (struct mpipe_tcp_server_src *)self;
	int ret = mpipe_src_change_state(self, transition);

	if (ret != 0) {
		return ret;
	}

	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		ret = mpipe_net_tcp_listen(tsrc->port);
		if (ret < 0) {
			return ret;
		}
		tsrc->server_fd = ret;
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_PLAYING:
		/* A pause keeps the client, so accept only on the first start */
		if (tsrc->client_fd < 0) {
			ret = mpipe_net_tcp_accept(tsrc->server_fd);
			if (ret < 0) {
				return ret;
			}
			tsrc->client_fd = ret;
		}
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		mpipe_net_tcp_close(&tsrc->client_fd);
		mpipe_net_tcp_close(&tsrc->server_fd);
		break;
	default:
		break;
	}

	return 0;
}

int mpipe_tcp_server_src_init(struct mpipe_tcp_server_src *tsrc, uint8_t id)
{
	const struct mpipe_buffer_pool_config pool_req = {
		.size = CONFIG_MPIPE_NET_SRC_BUF_SIZE,
		.min_buffers = 1,
	};
	struct mpipe_element *self = &tsrc->src.element;
	int ret = mpipe_src_init(&tsrc->src, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "tcp_server_src");

	self->object.set_property = mpipe_tcp_server_src_set_property;
	self->object.get_property = mpipe_tcp_server_src_get_property;
	self->change_state = mpipe_tcp_server_src_change_state;

	mpipe_buffer_pool_init(&tsrc->pool);
	tsrc->pool.nb_pool = &mpipe_tcp_server_src_nb_pool;
	(void)mpipe_buffer_pool_set_req_config(&tsrc->pool, &pool_req);
	tsrc->pool.acquire_buffer = mpipe_tcp_server_src_pool_acquire;
	tsrc->src.pool = &tsrc->pool;

	tsrc->port = CONFIG_MPIPE_NET_SRC_PORT;
	tsrc->server_fd = -1;
	tsrc->client_fd = -1;

	return 0;
}
