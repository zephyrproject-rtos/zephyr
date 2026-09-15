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
#include <zephyr/mp/core/mp_pipeline.h>
#include <zephyr/mp/ipc/mp_ipc.h>
#include <zephyr/mp/ipc/mp_ipc_protocol.h>

LOG_MODULE_REGISTER(mp_ipc_src, CONFIG_MP_LOG_LEVEL);

__weak int app_create_remote_pipeline(void)
{
	return 0;
}

__weak int app_destroy_remote_pipeline(void)
{
	return 0;
}

/* Pool for buffer wrappers on the source side */
NET_BUF_POOL_DEFINE(ipc_src_pool, MP_IPC_MAX_BUFFERS, 0, sizeof(struct mp_buffer_meta), NULL);

static struct mp_ipc_src *global_ipc_src = NULL;

static int ipc_src_release_buffer(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	struct mp_ipc_src *src = global_ipc_src;
	struct mp_buffer_meta *meta = mp_buffer_get_meta(buf);
	uint32_t buf_id = (uint32_t)(uintptr_t)meta->priv;

	if (src && src->bound) {
		struct mp_ipc_msg msg = {
			.type = IPC_MSG_DATA_RELEASE,
			.release = {
				.buffer_id = buf_id
			}
		};
		ipc_service_send(&src->ept, &msg, sizeof(msg));
		LOG_DBG("Sent release for buffer id %u", buf_id);
	}

	return 0;
}

static struct mp_buffer_pool ipc_src_mp_pool = {
	.nb_pool = &ipc_src_pool,
	.release_buffer = ipc_src_release_buffer,
};

static void ipc_bound_cb(void *priv)
{
	struct mp_ipc_src *src = priv;
	LOG_INF("IPC src endpoint bound: %s", src->endpoint_name);
	src->bound = true;
}

static void ipc_received_cb(const void *data, size_t len, void *priv)
{
	struct mp_ipc_src *src = priv;
	const struct mp_ipc_msg *msg = data;

	if (len != sizeof(struct mp_ipc_msg)) {
		LOG_ERR("Received invalid IPC message length: %zu", len);
		return;
	}

	if (msg->type == IPC_MSG_CMD_CREATE_PIPELINE) {
		LOG_INF("CREATE_PIPELINE received");
		app_create_remote_pipeline();
		return;
	} else if (msg->type == IPC_MSG_CMD_DESTROY_PIPELINE) {
		LOG_INF("DESTROY_PIPELINE received");
		app_destroy_remote_pipeline();
		return;
	}

	if (msg->type == IPC_MSG_CMD_STATE_CHANGE) {
		LOG_INF("Received state change command: %u", msg->state_cmd.state);
		if (src->base.element.object.container) {
			mp_element_set_state((struct mp_element *)src->base.element.object.container, msg->state_cmd.state);
		}
	} else if (msg->type == IPC_MSG_DATA_BUFFER) {
		LOG_DBG("Received buffer id %u", msg->data.buffer_id);

		struct net_buf *buf = net_buf_alloc(&ipc_src_pool, K_NO_WAIT);
		if (!buf) {
			LOG_ERR("Failed to allocate IPC wrapper buffer");
			struct mp_ipc_msg rel_msg = {
				.type = IPC_MSG_DATA_RELEASE,
				.release = { .buffer_id = msg->data.buffer_id }
			};
			ipc_service_send(&src->ept, &rel_msg, sizeof(rel_msg));
			return;
		}

		/* Zero-copy: Point to the shared memory */
		buf->data = (uint8_t *)(uintptr_t)msg->data.phys_addr;
		buf->len = msg->data.size;
		buf->size = msg->data.size;

#if defined(CONFIG_CACHE_MANAGEMENT)
		sys_cache_data_invd_range(buf->data, buf->len);
#endif

		struct mp_buffer_meta *meta = mp_buffer_get_meta(buf);
		meta->timestamp = msg->data.timestamp;
		meta->pool = &ipc_src_mp_pool;
		meta->priv = (void *)(uintptr_t)msg->data.buffer_id;

		/* Push to downstream */
		mp_pipeline_push_buffer(&src->base.srcpad, buf);
	} else if (msg->type == IPC_MSG_EVENT) {
		LOG_DBG("Received downstream event from ipc_sink");
		struct mp_dispatch event;
		mp_dispatch_init(&event, msg->event.event_type, NULL);
		mp_pad_send_event(src->base.srcpad.peer, &event);
		mp_dispatch_clear(&event);
	} else if (msg->type == IPC_MSG_QUERY_REQ) {
		LOG_DBG("Received downstream query request from ipc_sink");
		struct mp_dispatch local_query;
		bool has_query = false;

		if (msg->query.query_type == MP_DISPATCH_CAPS) {
			mp_dispatch_caps_init(&local_query, NULL);
			has_query = true;
		} else if (msg->query.query_type == MP_DISPATCH_BUFFER_CONFIG) {
			mp_dispatch_buffer_config_init(&local_query, NULL);
			has_query = true;
		}

		int res = -EINVAL;
		if (has_query) {
			res = mp_pad_query(src->base.srcpad.peer, &local_query);
			mp_dispatch_clear(&local_query);
		}

		/* Always send a response back to prevent peer thread query timeout */
		struct mp_ipc_msg resp_msg = {
			.type = IPC_MSG_QUERY_RESP,
			.query = {
				.query_type = msg->query.query_type,
				.success = (has_query && res == 0)
			}
		};
		ipc_service_send(&src->ept, &resp_msg, sizeof(resp_msg));
	} else if (msg->type == IPC_MSG_QUERY_RESP) {
		LOG_DBG("Received query response from ipc_sink (upstream)");
		src->last_query_success = msg->query.success;
		/* Unblock waiting upstream query */
		k_sem_give(&src->query_sem);
	} else {
		LOG_WRN("Unhandled IPC message type: %u", msg->type);
	}
}

static int mp_ipc_src_eventfn(struct mp_pad *pad, struct mp_dispatch *event)
{
	struct mp_ipc_src *src = (struct mp_ipc_src *)pad->object.container;

	if (!src->bound) {
		return -ENOTCONN;
	}

	LOG_DBG("Sending upstream event to ipc_sink: type %u", event->type);

	struct mp_ipc_msg msg = {
		.type = IPC_MSG_EVENT,
		.event = {
			.event_type = event->type
		}
	};

	int ret = ipc_service_send(&src->ept, &msg, sizeof(msg));
	if (ret < 0) {
		LOG_ERR("Failed to send event message: %d", ret);
		return ret;
	}

	return 0;
}

static int mp_ipc_src_queryfn(struct mp_pad *pad, struct mp_dispatch *query)
{
	struct mp_ipc_src *src = (struct mp_ipc_src *)pad->object.container;

	if (!src->bound) {
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

	LOG_DBG("Sending upstream query to ipc_sink: type %u", query->type);

	struct mp_ipc_msg msg = {
		.type = IPC_MSG_QUERY_REQ,
		.query = {
			.query_type = query->type
		}
	};

	src->last_query_success = false;

	int ret = ipc_service_send(&src->ept, &msg, sizeof(msg));
	if (ret < 0) {
		LOG_ERR("Failed to send query message: %d", ret);
		return ret;
	}

	if (k_sem_take(&src->query_sem, K_MSEC(5000)) != 0) {
		LOG_ERR("Query timed out waiting for ipc_sink");
		return -ETIMEDOUT;
	}

	if (src->last_query_success) {
		return 0;
	}

	return -EIO;
}

static enum mp_state_change_return mp_ipc_src_change_state(struct mp_element *element,
							     enum mp_state_change transition)
{
	struct mp_ipc_src *src = (struct mp_ipc_src *)element;
	enum mp_state_change_return ret = MP_STATE_CHANGE_SUCCESS;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (!src->bound) {
			static struct ipc_ept_cfg ept_cfg;
			ept_cfg.name = src->endpoint_name;
			ept_cfg.cb.bound = ipc_bound_cb;
			ept_cfg.cb.received = ipc_received_cb;
			ept_cfg.priv = src;

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

			err = ipc_service_register_endpoint(ipc_instance, &src->ept, &ept_cfg);
			if (err) {
				LOG_ERR("Failed to register IPC endpoint: %d", err);
				return MP_STATE_CHANGE_FAILURE;
			}
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		break;
	case MP_STATE_CHANGE_PLAYING_TO_PAUSED:
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		break;
	default:
		break;
	}

	return ret;
}

void mp_ipc_src_init(struct mp_element *self)
{
	struct mp_ipc_src *src = (struct mp_ipc_src *)self;
	struct mp_src *src_base = (struct mp_src *)self;

	mp_src_init(self);

	self->change_state = mp_ipc_src_change_state;
	src_base->srcpad.eventfn = mp_ipc_src_eventfn;
	src_base->srcpad.queryfn = mp_ipc_src_queryfn;

	src->bound = false;
	strncpy(src->endpoint_name, "mp_ipc", sizeof(src->endpoint_name) - 1);

	global_ipc_src = src;
	k_sem_init(&src->query_sem, 0, 1);
}
