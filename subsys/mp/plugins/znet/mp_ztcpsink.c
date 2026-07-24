/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/znet/mp_ztcpsink.h>

LOG_MODULE_REGISTER(mp_ztcpsink, CONFIG_MP_LOG_LEVEL);

static int mp_ztcpsink_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_ztcpsink *tsink = MP_ZTCPSINK(obj);

	switch (key) {
	case PROP_ZTCPSINK_PORT:
		tsink->port = *(const uint16_t *)val;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_ztcpsink_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_ztcpsink *tsink = MP_ZTCPSINK(obj);

	switch (key) {
	case PROP_ZTCPSINK_PORT:
		*(uint16_t *)val = tsink->port;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_ztcpsink_chainfn(struct mp_pad *pad, struct net_buf *in_buf, struct net_buf **out_buf)
{
	struct mp_ztcpsink *tsink = MP_ZTCPSINK(pad->object.container);
	struct net_buf *frag;
	uint32_t bytes_sent = 0;

	*out_buf = NULL;

	if (tsink->client_fd < 0) {
		LOG_ERR("No client connected");
		net_buf_unref(in_buf);
		return -ENOTCONN;
	}

	for (frag = in_buf; frag != NULL; frag = frag->frags) {
		uint32_t len = mp_buffer_get_meta(frag)->bytes_used;
		uint32_t frag_sent = 0;

		if (len == 0) {
			continue;
		}

		while (frag_sent < len) {
			int sent = zsock_send(tsink->client_fd,
					      frag->data + frag_sent,
					      len - frag_sent, 0);

			if (sent < 0) {
				LOG_ERR("send() failed: %d", errno);
				net_buf_unref(in_buf);
				return -EIO;
			}
			frag_sent += (uint32_t)sent;
		}
		bytes_sent += frag_sent;
	}

	LOG_DBG("sent %u bytes to client", bytes_sent);
	net_buf_unref(in_buf);
	return 0;
}

static int ztcpsink_open_listeners(struct mp_ztcpsink *tsink)
{
	int opt;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct sockaddr_in6 addr6 = {
			.sin6_family = AF_INET6,
			.sin6_port   = htons(tsink->port),
			.sin6_addr   = in6addr_any,
		};

		tsink->server_fd = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (tsink->server_fd < 0) {
			LOG_ERR("socket() failed: %d", errno);
			return -errno;
		}

		/* Disable V6ONLY so IPv4-mapped addresses are accepted on the same socket */
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			opt = 0;
			zsock_setsockopt(tsink->server_fd, IPPROTO_IPV6, IPV6_V6ONLY,
					 &opt, sizeof(opt));
		}

		opt = 1;
		zsock_setsockopt(tsink->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (zsock_bind(tsink->server_fd,
			       (struct sockaddr *)&addr6, sizeof(addr6)) < 0) {
			LOG_ERR("bind() on port %u failed: %d", tsink->port, errno);
			zsock_close(tsink->server_fd);
			tsink->server_fd = -1;
			return -errno;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && !IS_ENABLED(CONFIG_NET_IPV6)) {
		struct sockaddr_in addr4 = {
			.sin_family      = AF_INET,
			.sin_port        = htons(tsink->port),
			.sin_addr.s_addr = INADDR_ANY,
		};

		tsink->server_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tsink->server_fd < 0) {
			LOG_ERR("socket() failed: %d", errno);
			return -errno;
		}

		opt = 1;
		zsock_setsockopt(tsink->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (zsock_bind(tsink->server_fd,
			       (struct sockaddr *)&addr4, sizeof(addr4)) < 0) {
			LOG_ERR("bind() on port %u failed: %d", tsink->port, errno);
			zsock_close(tsink->server_fd);
			tsink->server_fd = -1;
			return -errno;
		}
	} else {
		LOG_ERR("Neither IPv4 nor IPv6 is configured");
		return -ENOTSUP;
	}

	if (zsock_listen(tsink->server_fd, 1) < 0) {
		LOG_ERR("listen() on port %u failed: %d", tsink->port, errno);
		zsock_close(tsink->server_fd);
		tsink->server_fd = -1;
		return -errno;
	}

	LOG_DBG("Waiting for client on port %u", tsink->port);

	tsink->client_fd = zsock_accept(tsink->server_fd, NULL, NULL);
	if (tsink->client_fd < 0) {
		LOG_ERR("accept() failed: %d", errno);
		zsock_close(tsink->server_fd);
		tsink->server_fd = -1;
		return -errno;
	}

	LOG_DBG("Client connected");
	return 0;
}

static enum mp_state_change_return mp_ztcpsink_change_state(struct mp_element *self,
							    enum mp_state_change transition)
{
	struct mp_ztcpsink *tsink = MP_ZTCPSINK(self);

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (ztcpsink_open_listeners(tsink) < 0) {
			return MP_STATE_CHANGE_FAILURE;
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		if (tsink->client_fd >= 0) {
			zsock_close(tsink->client_fd);
			tsink->client_fd = -1;
		}
		if (tsink->server_fd >= 0) {
			zsock_close(tsink->server_fd);
			tsink->server_fd = -1;
		}
		LOG_DBG("TCP connections closed");
		break;
	default:
		break;
	}

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_ztcpsink_init(struct mp_element *self)
{
	struct mp_sink *sink = (struct mp_sink *)self;
	struct mp_ztcpsink *tsink = MP_ZTCPSINK(self);

	mp_sink_init(self);

	self->object.set_property = mp_ztcpsink_set_property;
	self->object.get_property = mp_ztcpsink_get_property;
	self->change_state = mp_ztcpsink_change_state;

	sink->sinkpad.chainfn = mp_ztcpsink_chainfn;

	tsink->port      = CONFIG_MP_PLUGIN_ZNET_DEFAULT_PORT;
	tsink->server_fd = -1;
	tsink->client_fd = -1;
}
