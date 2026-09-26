/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/net/mpipe_tcp_server_sink.h>

#include "tcp_server.h"

LOG_MODULE_REGISTER(mpipe_tcp_server_sink, CONFIG_MPIPE_LOG_LEVEL);

static int mpipe_tcp_server_sink_set_property(struct mpipe_object *obj, uint32_t key,
					      const void *val)
{
	struct mpipe_tcp_server_sink *tsink = (struct mpipe_tcp_server_sink *)obj;

	switch (key) {
	case MPIPE_PROP_TCP_SERVER_SINK_PORT:
		if (val == NULL) {
			return -EINVAL;
		}
		tsink->port = *(const uint16_t *)val;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_tcp_server_sink_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_tcp_server_sink *tsink = (struct mpipe_tcp_server_sink *)obj;

	switch (key) {
	case MPIPE_PROP_TCP_SERVER_SINK_PORT:
		*(uint16_t *)val = tsink->port;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_tcp_server_sink_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
					  struct net_buf **out_buf)
{
	struct mpipe_tcp_server_sink *tsink = CONTAINER_OF(
		pad->object.container, struct mpipe_tcp_server_sink, sink.element.object);
	int ret = 0;

	*out_buf = NULL;

	if (tsink->client_fd < 0) {
		ret = -ENOTCONN;
	}

	/* A parser may hand over several frames as a fragment chain */
	for (struct net_buf *frag = in_buf; frag != NULL && ret == 0; frag = frag->frags) {
		uint32_t len = mpipe_buffer_get_meta(frag)->bytes_used;
		uint32_t sent = 0;

		while (sent < len) {
			ssize_t wr = zsock_send(tsink->client_fd, frag->data + sent, len - sent, 0);

			if (wr < 0) {
				ret = -errno;
				LOG_ERR("send() failed (%d)", errno);
				break;
			}
			sent += wr;
		}
	}

	net_buf_unref(in_buf);

	return ret;
}

static int mpipe_tcp_server_sink_change_state(struct mpipe_element *self,
					      enum mpipe_state_change transition)
{
	struct mpipe_tcp_server_sink *tsink = (struct mpipe_tcp_server_sink *)self;
	int ret;

	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		ret = mpipe_net_tcp_listen(tsink->port);
		if (ret < 0) {
			return ret;
		}
		tsink->server_fd = ret;
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_PLAYING:
		/* A pause keeps the client, so accept only on the first start */
		if (tsink->client_fd < 0) {
			ret = mpipe_net_tcp_accept(tsink->server_fd);
			if (ret < 0) {
				return ret;
			}
			tsink->client_fd = ret;
		}
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		mpipe_net_tcp_close(&tsink->client_fd);
		mpipe_net_tcp_close(&tsink->server_fd);
		break;
	default:
		break;
	}

	return mpipe_sink_change_state(self, transition);
}

int mpipe_tcp_server_sink_init(struct mpipe_tcp_server_sink *tsink, uint8_t id)
{
	struct mpipe_element *self = &tsink->sink.element;
	int ret = mpipe_sink_init(&tsink->sink, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "tcp_server_sink");

	self->object.set_property = mpipe_tcp_server_sink_set_property;
	self->object.get_property = mpipe_tcp_server_sink_get_property;
	self->change_state = mpipe_tcp_server_sink_change_state;
	tsink->sink.sink_pad.chain_fn = mpipe_tcp_server_sink_chain_fn;

	tsink->port = CONFIG_MPIPE_NET_SINK_PORT;
	tsink->server_fd = -1;
	tsink->client_fd = -1;

	return 0;
}
