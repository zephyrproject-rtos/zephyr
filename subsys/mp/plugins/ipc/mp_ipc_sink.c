/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/cache.h>

#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_message.h>
#include <zephyr/mp/core/mp_bus.h>
#include <zephyr/mp/ipc/mp_ipc.h>
#include <zephyr/mp/ipc/mp_ipc_protocol.h>

LOG_MODULE_REGISTER(mp_ipc_sink, CONFIG_MP_LOG_LEVEL);
K_SEM_DEFINE(ipc_bound_sem, 0, 1);

static void ipc_bound_cb(void *priv)
{
	struct mp_ipc_sink *sink = priv;
	LOG_INF("IPC sink endpoint bound: %s", sink->endpoint_name);
	sink->bound = true;
	k_sem_give(&ipc_bound_sem);
}

static void ipc_received_cb(const void *data, size_t len, void *priv)
{
	struct mp_ipc_sink *sink = priv;
	const struct mp_ipc_msg *msg = data;

	if (len != sizeof(struct mp_ipc_msg)) {
		LOG_ERR("Received invalid IPC message length: %zu", len);
		return;
	}

	if (msg->type == IPC_MSG_DATA_RELEASE) {
		uint32_t id = msg->release.buffer_id;

		if (id < MP_IPC_MAX_BUFFERS && sink->pending_buffers[id] != NULL) {
			struct net_buf *buf = sink->pending_buffers[id];
			sink->pending_buffers[id] = NULL;
			net_buf_unref(buf);
			LOG_DBG("Released buffer id %u", id);
		} else {
			LOG_ERR("Received release for invalid buffer id: %u", id);
		}
	} else if (msg->type == IPC_MSG_QUERY_RESP) {
		LOG_DBG("Received query response: %u", msg->query.success);
		sink->last_query_success = msg->query.success;
		/* Unblock the thread waiting for the query response */
		k_sem_give(&sink->query_sem);
	} else if (msg->type == IPC_MSG_EVENT) {
		LOG_DBG("Received upstream event from ipc_src");
		/* This is an event from P2 flowing upstream to P1 */
		struct mp_dispatch event;
		mp_dispatch_init(&event, msg->event.event_type, NULL);
		mp_pad_send_event(sink->base.sinkpad.peer, &event);
		mp_dispatch_clear(&event);
	} else if (msg->type == IPC_MSG_BUS_MESSAGE) {
		LOG_DBG("Received bus message from ipc_src");
		/* Forward the bus message to P1's pipeline bus */
		if (sink->base.element.object.container) {
			struct mp_bus *bus = mp_element_get_bus((struct mp_element *)sink);
			if (bus) {
				struct mp_message bus_msg;
				MP_MESSAGE_INIT(&bus_msg, (struct mp_element *)sink, msg->bus_msg.msg_type);
				mp_bus_post(bus, &bus_msg);
			}
		}
	} else {
		LOG_WRN("Unhandled IPC message type: %u", msg->type);
	}
}

static int mp_ipc_sink_chainfn(struct mp_pad *pad, struct net_buf *in_buf, struct net_buf **out_buf)
{
	struct mp_ipc_sink *sink = (struct mp_ipc_sink *)pad->object.container;
	struct mp_buffer_meta *meta = mp_buffer_get_meta(in_buf);
	int buf_id = -1;

	if (!sink->bound) {
		LOG_ERR("Cannot send buffer, IPC endpoint not bound");
		if (out_buf) {
			*out_buf = NULL;
		}
		return -ENOTCONN;
	}

	for (int i = 0; i < MP_IPC_MAX_BUFFERS; i++) {
		if (sink->pending_buffers[i] == NULL) {
			buf_id = i;
			break;
		}
	}

	if (buf_id < 0) {
		LOG_ERR("No free pending buffer slots");
		if (out_buf) {
			*out_buf = NULL;
		}
		return -ENOMEM;
	}

	sink->pending_buffers[buf_id] = net_buf_ref(in_buf);

#if defined(CONFIG_CACHE_MANAGEMENT)
	sys_cache_data_flush_range(in_buf->data, in_buf->len);
#endif

	struct mp_ipc_msg msg = {
		.type = IPC_MSG_DATA_BUFFER,
		.data = {
			.phys_addr = (uint32_t)(uintptr_t)in_buf->data,
			.size = in_buf->len,
			.timestamp = meta->timestamp,
			.buffer_id = buf_id
		}
	};

	int ret = ipc_service_send(&sink->ept, &msg, sizeof(msg));
	if (ret < 0) {
		LOG_ERR("Failed to send IPC message: %d", ret);
		net_buf_unref(sink->pending_buffers[buf_id]);
		sink->pending_buffers[buf_id] = NULL;
		if (out_buf) {
			*out_buf = NULL;
		}
		return ret;
	}

	LOG_DBG("Sent buffer id %d", buf_id);

	if (out_buf) {
		*out_buf = NULL;
	}

	return 0;
}

static int mp_ipc_sink_eventfn(struct mp_pad *pad, struct mp_dispatch *event)
{
	struct mp_ipc_sink *sink = (struct mp_ipc_sink *)pad->object.container;

	if (!sink->bound) {
		return -ENOTCONN;
	}

	LOG_DBG("Sending downstream event to ipc_src: type %u", event->type);

	struct mp_ipc_msg msg = {
		.type = IPC_MSG_EVENT,
		.event = {
			.event_type = event->type
		}
	};

	int ret = ipc_service_send(&sink->ept, &msg, sizeof(msg));
	if (ret < 0) {
		LOG_ERR("Failed to send event message: %d", ret);
		return ret;
	}

	return 0;
}

static int mp_ipc_sink_queryfn(struct mp_pad *pad, struct mp_dispatch *query)
{
	struct mp_ipc_sink *sink = (struct mp_ipc_sink *)pad->object.container;

	if (!sink->bound) {
		if (query->type == MP_DISPATCH_CAPS) {
			struct mp_caps *caps = mp_caps_new_any();
			mp_dispatch_set_caps(query, caps);
			mp_caps_unref(caps);
			return 0;
		} else if (query->type == MP_DISPATCH_BUFFER_CONFIG) {
			return 0;
		}
		return -ENOTCONN;
	}

	LOG_DBG("Sending query to ipc_src: type %u", query->type);

	struct mp_ipc_msg msg = {
		.type = IPC_MSG_QUERY_REQ,
		.query = {
			.query_type = query->type
		}
	};

	sink->last_query_success = false;

	int ret = ipc_service_send(&sink->ept, &msg, sizeof(msg));
	if (ret < 0) {
		LOG_ERR("Failed to send query message: %d", ret);
		return ret;
	}

	/* Block until P2 replies */
	if (k_sem_take(&sink->query_sem, K_MSEC(5000)) != 0) {
		LOG_ERR("Query timed out waiting for ipc_src");
		return -ETIMEDOUT;
	}

	if (sink->last_query_success) {
		return 0;
	}

	return -EIO;
}

static enum mp_state_change_return mp_ipc_sink_change_state(struct mp_element *element,
							      enum mp_state_change transition)
{
	struct mp_ipc_sink *sink = (struct mp_ipc_sink *)element;
	enum mp_state_change_return ret = MP_STATE_CHANGE_SUCCESS;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (!sink->bound) {
			static struct ipc_ept_cfg ept_cfg;
			ept_cfg.name = sink->endpoint_name;
			ept_cfg.cb.bound = ipc_bound_cb;
			ept_cfg.cb.received = ipc_received_cb;
			ept_cfg.priv = sink;
			const struct device *ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
			if (!device_is_ready(ipc_instance)) {
				LOG_ERR("IPC instance not ready");
				return MP_STATE_CHANGE_FAILURE;
			}

			int err = ipc_service_open_instance(ipc_instance);
			if (err < 0 && err != -EALREADY) {
				LOG_ERR("Failed to open IPC instance: %d", err);
				return MP_STATE_CHANGE_FAILURE;
			}

			err = ipc_service_register_endpoint(ipc_instance, &sink->ept, &ept_cfg);
			if (err) {
				LOG_ERR("Failed to register IPC endpoint: %d", err);
				return MP_STATE_CHANGE_FAILURE;
			}
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		LOG_INF("PAUSED_TO_PLAYING bound=%d", sink->bound);
		if (sink->bound) {
			struct mp_ipc_msg msg = {
				.type = IPC_MSG_CMD_STATE_CHANGE,
				.state_cmd = {
					.state = MP_STATE_PLAYING
				}
			};
			int err = ipc_service_send(&sink->ept, &msg, sizeof(msg));
			if (err < 0) {
				LOG_ERR("Failed to send state change to ipc_src: %d", err);
			}
		}
		break;
	case MP_STATE_CHANGE_PLAYING_TO_PAUSED:
		if (sink->bound) {
			struct mp_ipc_msg msg = {
				.type = IPC_MSG_CMD_STATE_CHANGE,
				.state_cmd = {
					.state = MP_STATE_PAUSED
				}
			};
			ipc_service_send(&sink->ept, &msg, sizeof(msg));
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		break;
	default:
		break;
	}

	return ret;
}

void mp_ipc_sink_init(struct mp_element *self)
{
	struct mp_ipc_sink *sink = (struct mp_ipc_sink *)self;
	struct mp_sink *sink_base = (struct mp_sink *)self;

	mp_sink_init(self);

	self->change_state = mp_ipc_sink_change_state;
	sink_base->sinkpad.chainfn = mp_ipc_sink_chainfn;
	sink_base->sinkpad.eventfn = mp_ipc_sink_eventfn;
	sink_base->sinkpad.queryfn = mp_ipc_sink_queryfn;

	sink->bound = false;
	strncpy(sink->endpoint_name, "mp_ipc", sizeof(sink->endpoint_name) - 1);

	for (int i = 0; i < MP_IPC_MAX_BUFFERS; i++) {
		sink->pending_buffers[i] = NULL;
	}

	k_sem_init(&sink->query_sem, 0, 1);
}

