/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net_buf.h>

#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/mp_caps.h>

#include <zephyr/mp/net/mp_tcpsrc.h>

#include "tcp_server.h"

LOG_MODULE_REGISTER(mp_tcpsrc, CONFIG_MP_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mp_net_src_pool, CONFIG_MP_NET_SRC_NUM_BUFS, CONFIG_MP_NET_SRC_BUF_SIZE,
			  sizeof(struct mp_buffer_meta), mp_buffer_destroy);

static int mp_tcpsrc_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_tcpsrc *tsrc = (struct mp_tcpsrc *)obj;

	switch (key) {
	case MP_PROP_NET_SRC_PORT:
		if (val == NULL) {
			return -EINVAL;
		}
		tsrc->port = *(const uint16_t *)val;
		return 0;
	default:
		return mp_src_set_property(obj, key, val);
	}
}

static int mp_tcpsrc_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_tcpsrc *tsrc = (struct mp_tcpsrc *)obj;

	switch (key) {
	case MP_PROP_NET_SRC_PORT:
		*(uint16_t *)val = tsrc->port;
		return 0;
	default:
		return mp_src_get_property(obj, key, val);
	}
}

static int mp_tcpsrc_pool_acquire_buffer(struct mp_buffer_pool *pool, struct net_buf **buf)
{
	struct mp_tcpsrc *tsrc = CONTAINER_OF(pool, struct mp_tcpsrc, pool);
	struct mp_buffer_meta *meta;
	struct net_buf *out;
	ssize_t rd;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (tsrc->client_fd < 0) {
		return -ENOTCONN;
	}

	out = net_buf_alloc_len(&mp_net_src_pool, CONFIG_MP_NET_SRC_BUF_SIZE, K_NO_WAIT);
	if (out == NULL) {
		return -ENOBUFS;
	}

	meta = mp_buffer_get_meta(out);
	meta->pool = pool;
	meta->bytes_used = 0;
	meta->timestamp = 0;
	meta->driver_buf = NULL;
	meta->priv = NULL;

	rd = zsock_recv(tsrc->client_fd, out->data, out->size, 0);
	if (rd == 0) {
		LOG_DBG("Client disconnected");
		net_buf_unref(out);
		return -ENODATA;
	}

	if (rd < 0) {
		LOG_ERR("recv() failed (%d)", errno);
		net_buf_unref(out);
		return -errno;
	}

	meta->bytes_used = (uint32_t)rd;
	out->len = (uint32_t)rd;
	*buf = out;

	LOG_DBG("Received %u bytes from the client", (uint32_t)rd);

	return 0;
}

static int mp_tcpsrc_pool_release_buffer(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	struct mp_buffer_meta *meta;

	ARG_UNUSED(pool);

	if (buf == NULL) {
		return -EINVAL;
	}

	meta = mp_buffer_get_meta(buf);
	meta->bytes_used = 0;
	meta->timestamp = 0;
	meta->driver_buf = NULL;
	meta->priv = NULL;
	buf->len = 0;

	return 0;
}

static enum mp_state_change_return mp_tcpsrc_change_state(struct mp_element *self,
							  enum mp_state_change transition)
{
	struct mp_tcpsrc *tsrc = (struct mp_tcpsrc *)self;
	enum mp_state_change_return ret;

	/* Reuse base mp_src negotiation/pool start behavior */
	ret = mp_src_change_state(self, transition);
	if (ret != MP_STATE_CHANGE_SUCCESS) {
		return ret;
	}

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (mp_net_tcp_accept_one(tsrc->port, &tsrc->server_fd, &tsrc->client_fd) != 0) {
			return MP_STATE_CHANGE_FAILURE;
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		mp_net_tcp_close(&tsrc->server_fd, &tsrc->client_fd);
		LOG_DBG("TCP connections closed");
		break;
	default:
		break;
	}

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_tcpsrc_init(struct mp_element *self)
{
	struct mp_tcpsrc *tsrc = (struct mp_tcpsrc *)self;
	struct mp_src *src = &tsrc->src;
	struct mp_caps *src_caps;

	mp_src_init(self);

	self->object.set_property = mp_tcpsrc_set_property;
	self->object.get_property = mp_tcpsrc_get_property;
	self->change_state = mp_tcpsrc_change_state;

	src_caps = mp_caps_new_any();
	mp_src_update_caps(src, src_caps);
	mp_caps_unref(src_caps);

	src->pool = &tsrc->pool;

	mp_buffer_pool_init(&tsrc->pool);
	tsrc->pool.config.size = CONFIG_MP_NET_SRC_BUF_SIZE;
	tsrc->pool.acquire_buffer = mp_tcpsrc_pool_acquire_buffer;
	tsrc->pool.release_buffer = mp_tcpsrc_pool_release_buffer;

	tsrc->port = CONFIG_MP_NET_SRC_PORT;
	tsrc->server_fd = -1;
	tsrc->client_fd = -1;
}
