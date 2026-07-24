/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mp/core/mp_bus.h>
#include <zephyr/mp/core/mp_message.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>

#include "mp_test_sink.h"

LOG_MODULE_DECLARE(mp_ipc_remote_app, LOG_LEVEL_INF);

static int mp_test_sink_chainfn(struct mp_pad *pad, struct net_buf *buf, struct net_buf **out_buf)
{
	struct mp_test_sink *self = (struct mp_test_sink *)pad->object.container;
	struct mp_buffer_meta *meta = mp_buffer_get_meta(buf);

	self->frame_count++;
	LOG_INF("[Remote Core: mp_test_sink] Received Frame %d over Real IPC: size=%u, timestamp=%u", 
		self->frame_count, buf->len, meta->timestamp);

	/* When we receive 5 frames, post an EOS message to Pipeline B's bus */
	if (self->frame_count >= 5) {
		struct mp_bus *bus = mp_element_get_bus((struct mp_element *)self);
		if (bus) {
			struct mp_message eos_msg;
			MP_MESSAGE_INIT(&eos_msg, (struct mp_element *)self, MP_MESSAGE_EOS);
			mp_bus_post(bus, &eos_msg);
		}
	}

	/* Unreference/consume the received buffer to free it and signal release back to CPU0 */
	net_buf_unref(buf);

	if (out_buf) {
		*out_buf = NULL;
	}
	return 0;
}

static int mp_test_sink_queryfn(struct mp_pad *pad, struct mp_dispatch *query)
{
	if (query->type == MP_DISPATCH_CAPS) {
		struct mp_caps *caps = mp_caps_new_any();
		mp_dispatch_set_caps(query, caps);
		mp_caps_unref(caps);
		return 0;
	} else if (query->type == MP_DISPATCH_BUFFER_CONFIG) {
		return 0;
	}
	return -ENOTSUP;
}

void mp_test_sink_init(struct mp_element *self)
{
	struct mp_test_sink *tsink = (struct mp_test_sink *)self;
	mp_sink_init(self);
	tsink->base.sinkpad.chainfn = mp_test_sink_chainfn;
	tsink->base.sinkpad.queryfn = mp_test_sink_queryfn;
	tsink->frame_count = 0;
}