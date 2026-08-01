/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/mp_caps.h>

#include <zephyr/mp/net/mp_tcpsink.h>

#include "tcp_server.h"

LOG_MODULE_REGISTER(mp_tcpsink, CONFIG_MP_LOG_LEVEL);

static int mp_tcpsink_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_tcpsink *tsink = (struct mp_tcpsink *)obj;

	switch (key) {
	case MP_PROP_NET_SINK_PORT:
		if (val == NULL) {
			return -EINVAL;
		}
		tsink->port = *(const uint16_t *)val;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_tcpsink_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_tcpsink *tsink = (struct mp_tcpsink *)obj;

	switch (key) {
	case MP_PROP_NET_SINK_PORT:
		*(uint16_t *)val = tsink->port;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_tcpsink_chainfn(struct mp_pad *pad, struct net_buf *in_buf, struct net_buf **out)
{
	struct mp_tcpsink *tsink =
		CONTAINER_OF(pad->object.container, struct mp_tcpsink, sink.element.object);
	struct net_buf *frag;
	uint32_t total = 0;
	int ret = 0;

	*out = NULL;

	if (tsink->client_fd < 0) {
		LOG_ERR("No client connected");
		ret = -ENOTCONN;
		goto out;
	}

	for (frag = in_buf; frag != NULL; frag = frag->frags) {
		uint32_t len = mp_buffer_get_meta(frag)->bytes_used;
		uint32_t sent = 0;

		while (sent < len) {
			ssize_t wr = zsock_send(tsink->client_fd, frag->data + sent, len - sent, 0);

			if (wr <= 0) {
				ret = -errno;
				LOG_ERR("send() failed (%d)", errno);
				goto out;
			}

			sent += (uint32_t)wr;
		}

		total += sent;
	}

	LOG_DBG("Sent %u bytes to the client", total);

out:
	net_buf_unref(in_buf);

	return ret;
}

static enum mp_state_change_return mp_tcpsink_change_state(struct mp_element *self,
							   enum mp_state_change transition)
{
	struct mp_tcpsink *tsink = (struct mp_tcpsink *)self;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (mp_net_tcp_accept_one(tsink->port, &tsink->server_fd, &tsink->client_fd) != 0) {
			return MP_STATE_CHANGE_FAILURE;
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		mp_net_tcp_close(&tsink->server_fd, &tsink->client_fd);
		LOG_DBG("TCP connections closed");
		break;
	default:
		break;
	}

	/*
	 * Chain to the base sink change_state, which resets the negotiated pad
	 * caps back to the template caps on PAUSED_TO_READY so a subsequent
	 * re-negotiation starts fresh.
	 */
	return mp_sink_change_state(self, transition);
}

void mp_tcpsink_init(struct mp_element *self)
{
	struct mp_tcpsink *tsink = (struct mp_tcpsink *)self;
	struct mp_sink *sink = &tsink->sink;
	struct mp_caps *sink_caps;

	mp_sink_init(self);

	self->object.set_property = mp_tcpsink_set_property;
	self->object.get_property = mp_tcpsink_get_property;
	self->change_state = mp_tcpsink_change_state;

	sink_caps = mp_caps_new_any();
	mp_sink_update_caps(sink, sink_caps);
	mp_caps_unref(sink_caps);

	sink->sinkpad.chainfn = mp_tcpsink_chainfn;

	tsink->port = CONFIG_MP_NET_SINK_PORT;
	tsink->server_fd = -1;
	tsink->client_fd = -1;
}
