/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/znet/mp_ztcpsrc.h>

LOG_MODULE_REGISTER(mp_ztcpsrc, CONFIG_MP_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(mp_znet_src_pool, CONFIG_MP_PLUGIN_ZNET_SRC_NUM_BUFS,
			  CONFIG_MP_PLUGIN_ZNET_SRC_BUF_SIZE, sizeof(struct mp_buffer_meta),
			  mp_buffer_destroy);

static int mp_ztcpsrc_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_ztcpsrc *tsrc = MP_ZTCPSRC(obj);

	switch (key) {
	case PROP_ZTCPSRC_PORT:
		tsrc->port = *(const uint16_t *)val;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_ztcpsrc_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_ztcpsrc *tsrc = MP_ZTCPSRC(obj);

	switch (key) {
	case PROP_ZTCPSRC_PORT:
		*(uint16_t *)val = tsrc->port;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_ztcpsrc_pool_acquire_buffer(struct mp_buffer_pool *pool, struct net_buf **buf_out)
{
	struct mp_ztcpsrc *tsrc = CONTAINER_OF(pool, struct mp_ztcpsrc, pool);
	struct net_buf *buf;
	struct mp_buffer_meta *m;
	ssize_t rd;

	if (tsrc->client_fd < 0) {
		return -EINVAL;
	}

	buf = net_buf_alloc_len(&mp_znet_src_pool, CONFIG_MP_PLUGIN_ZNET_SRC_BUF_SIZE, K_NO_WAIT);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	m = mp_buffer_get_meta(buf);
	m->pool = pool;
	m->bytes_used = 0;
	m->timestamp = 0;
	m->priv = NULL;

	rd = zsock_recv(tsrc->client_fd, buf->data, buf->size, 0);
	if (rd <= 0) {
		net_buf_unref(buf);
		if (rd == 0) {
			LOG_DBG("Client disconnected");
			return -ENODATA;
		}
		return -errno;
	}

	m->bytes_used = (uint32_t)rd;
	buf->len = (uint32_t)rd;
	*buf_out = buf;

	LOG_DBG("Received %zd bytes from client", rd);
	return 0;
}

static int mp_ztcpsrc_pool_release_buffer(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	if (buf != NULL) {
		struct mp_buffer_meta *m = mp_buffer_get_meta(buf);

		if (m != NULL) {
			m->bytes_used = 0;
			m->timestamp = 0;
			m->priv = NULL;
		}
		buf->len = 0;
	}

	return 0;
}

static int mp_ztcpsrc_decide_allocation(struct mp_src *src, struct mp_dispatch *query)
{
	ARG_UNUSED(src);
	ARG_UNUSED(query);
	return 0;
}

static int ztcpsrc_open_listener(struct mp_ztcpsrc *tsrc)
{
	int opt;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct sockaddr_in6 addr6 = {
			.sin6_family = AF_INET6,
			.sin6_port   = htons(tsrc->port),
			.sin6_addr   = in6addr_any,
		};

		tsrc->server_fd = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (tsrc->server_fd < 0) {
			LOG_ERR("socket() failed: %d", errno);
			return -errno;
		}

		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			opt = 0;
			zsock_setsockopt(tsrc->server_fd, IPPROTO_IPV6, IPV6_V6ONLY,
					 &opt, sizeof(opt));
		}

		opt = 1;
		zsock_setsockopt(tsrc->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (zsock_bind(tsrc->server_fd,
			       (struct sockaddr *)&addr6, sizeof(addr6)) < 0) {
			LOG_ERR("bind() on port %u failed: %d", tsrc->port, errno);
			zsock_close(tsrc->server_fd);
			tsrc->server_fd = -1;
			return -errno;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && !IS_ENABLED(CONFIG_NET_IPV6)) {
		struct sockaddr_in addr4 = {
			.sin_family      = AF_INET,
			.sin_port        = htons(tsrc->port),
			.sin_addr.s_addr = INADDR_ANY,
		};

		tsrc->server_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tsrc->server_fd < 0) {
			LOG_ERR("socket() failed: %d", errno);
			return -errno;
		}

		opt = 1;
		zsock_setsockopt(tsrc->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (zsock_bind(tsrc->server_fd,
			       (struct sockaddr *)&addr4, sizeof(addr4)) < 0) {
			LOG_ERR("bind() on port %u failed: %d", tsrc->port, errno);
			zsock_close(tsrc->server_fd);
			tsrc->server_fd = -1;
			return -errno;
		}
	} else {
		LOG_ERR("Neither IPv4 nor IPv6 is configured");
		return -ENOTSUP;
	}

	if (zsock_listen(tsrc->server_fd, 1) < 0) {
		LOG_ERR("listen() on port %u failed: %d", tsrc->port, errno);
		zsock_close(tsrc->server_fd);
		tsrc->server_fd = -1;
		return -errno;
	}

	LOG_DBG("Waiting for client on port %u", tsrc->port);

	tsrc->client_fd = zsock_accept(tsrc->server_fd, NULL, NULL);
	if (tsrc->client_fd < 0) {
		LOG_ERR("accept() failed: %d", errno);
		zsock_close(tsrc->server_fd);
		tsrc->server_fd = -1;
		return -errno;
	}

	LOG_DBG("Client connected");
	return 0;
}

static enum mp_state_change_return mp_ztcpsrc_change_state(struct mp_element *self,
							   enum mp_state_change transition)
{
	struct mp_ztcpsrc *tsrc = MP_ZTCPSRC(self);
	enum mp_state_change_return ret;

	ret = mp_src_change_state(self, transition);
	if (ret != MP_STATE_CHANGE_SUCCESS) {
		return ret;
	}

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (ztcpsrc_open_listener(tsrc) < 0) {
			return MP_STATE_CHANGE_FAILURE;
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		if (tsrc->client_fd >= 0) {
			zsock_close(tsrc->client_fd);
			tsrc->client_fd = -1;
		}
		if (tsrc->server_fd >= 0) {
			zsock_close(tsrc->server_fd);
			tsrc->server_fd = -1;
		}
		LOG_DBG("TCP connections closed");
		break;
	default:
		break;
	}

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_ztcpsrc_init(struct mp_element *self)
{
	struct mp_src *src = (struct mp_src *)self;
	struct mp_ztcpsrc *tsrc = MP_ZTCPSRC(self);
	struct mp_caps *caps;

	mp_src_init(self);

	self->object.set_property = mp_ztcpsrc_set_property;
	self->object.get_property = mp_ztcpsrc_get_property;
	self->change_state = mp_ztcpsrc_change_state;

	caps = mp_caps_new_any();
	mp_src_update_caps(src, caps);
	mp_caps_unref(caps);

	src->decide_allocation = mp_ztcpsrc_decide_allocation;
	src->pool = &tsrc->pool;

	mp_buffer_pool_init(&tsrc->pool);
	tsrc->pool.config.size = CONFIG_MP_PLUGIN_ZNET_SRC_BUF_SIZE;
	tsrc->pool.acquire_buffer = mp_ztcpsrc_pool_acquire_buffer;
	tsrc->pool.release_buffer = mp_ztcpsrc_pool_release_buffer;

	tsrc->port      = CONFIG_MP_PLUGIN_ZNET_DEFAULT_SRC_PORT;
	tsrc->server_fd = -1;
	tsrc->client_fd = -1;
}
