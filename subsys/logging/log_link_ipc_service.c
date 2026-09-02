/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_link.h>
#include <zephyr/logging/log_ipc_service.h>
#include <zephyr/logging/log_link_ipc_service.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(link_ipc);

#if DT_HAS_CHOSEN(zephyr_log_ipc)
#define IPC_NODE DT_CHOSEN(zephyr_log_ipc)
#elif DT_CHILD_NUM_STATUS_OKAY(DT_PATH(ipc))
#define IPC_NODE DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(ipc), UTIL_EVAL)
#else
#error "No IPC node found"
#endif

#ifndef CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS
#define CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS 0
#endif

static int getter_msg_process(const struct log_link_ipc *link_ipc,
			      struct log_ipc_service_msg *msg, size_t msg_size)
{
	int err;
	struct log_link_ipc_data *data = link_ipc->data;

	err = ipc_service_send(&data->ept, msg, msg_size);
	if (err < 0) {
		return err;
	}

	err = k_sem_take(&data->rdy_sem, K_MSEC(1000));
	if (err < 0) {
		return err;
	}

	return (data->status == Z_LOG_IPC_SERVICE_STATUS_OK) ? 0 : -EIO;
}

static void dropped_timer_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	struct log_link_ipc *link_ipc = k_timer_user_data_get(timer);
	struct log_link_ipc_data *data = link_ipc->data;
	uint32_t cnt = (uint32_t)atomic_set(&data->dropped_cnt, 0);

	if (cnt > 0) {
		LOG_WRN("Remote: %s dropped messages: %d",
			link_ipc->link->name, cnt);
	}
}

static void msg_dropped(const struct log_link_ipc *link_ipc, uint32_t cnt)
{
	uint32_t prev = atomic_add(&link_ipc->data->dropped_cnt, cnt);
	k_timeout_t timeout = K_MSEC(CONFIG_LOG_LINK_IPC_SERVICE_DROPPED_MSG_TIMEOUT);

	if (prev == 0) {
		k_timer_start(&link_ipc->data->dropped_timer, timeout, K_NO_WAIT);
	}
}

static int link_ipc_initiate(const struct log_link *link,
				struct log_link_config *config)
{
	const struct log_link_ipc *link_ipc = link->ctx;
	int rv;

	k_sem_init(&link_ipc->data->rdy_sem, 0, 1);

	if (!IS_ENABLED(CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD) && link_ipc->buffer) {
		mpsc_pbuf_init(link_ipc->buffer, link_ipc->buffer_config);
	}

	rv = ipc_service_open_instance(link_ipc->ipc_instance);
	if (rv < 0 && rv != -EALREADY) {
		__ASSERT(0, "ipc_service_open_instance() failure (err:%d)\n", rv);
		return rv;
	}

	rv = ipc_service_register_endpoint(link_ipc->ipc_instance, &link_ipc->data->ept,
					     &link_ipc->ept_cfg);
	if (rv < 0) {
		return rv;
	}

	k_timer_init(&link_ipc->data->dropped_timer, dropped_timer_cb, NULL);
	k_timer_user_data_set(&link_ipc->data->dropped_timer, (void *)link_ipc);

	return 0;
}

static int link_remote_activate(const struct log_link *link)
{
	const struct log_link_ipc *link_ipc = link->ctx;

	if (!link_ipc->data->ready) {
		return -EINPROGRESS;
	}

	return link_ipc->data->status;
}

static int link_remote_get_source_name(const struct log_link *link, uint16_t source_id,
				       char *name, size_t *length)
{
	const struct log_link_ipc *link_ipc = link->ctx;
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME,
		.data = {
			.source_name = {
				.source_id = source_id
			}
		}
	};
	int err;

	link_ipc->data->rsp.name.rsp = name;
	link_ipc->data->rsp.name.len = *length;

	err = getter_msg_process(link_ipc, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	*length = link_ipc->data->rsp.name.len;
	return 0;
}

static int link_remote_get_levels(const struct log_link *link, uint16_t source_id,
				  uint8_t *level, uint8_t *runtime_level)
{
	const struct log_link_ipc *link_ipc = link->ctx;
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_GET_LEVELS,
		.data = {
			.levels = {
				.source_id = source_id
			}
		}
	};
	int err;

	err = getter_msg_process(link_ipc, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	if (level) {
		*level = link_ipc->data->rsp.levels.level;
	}
	if (runtime_level) {
		*runtime_level = link_ipc->data->rsp.levels.runtime_level;
	}

	return 0;
}

static int link_remote_set_runtime_level(const struct log_link *link, uint16_t source_id,
					 uint8_t level)
{
	const struct log_link_ipc *link_ipc = link->ctx;
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL,
		.data = {
			.set_rt_level = {
				.source_id = source_id,
				.runtime_level = level
			}
		}
	};
	int err;

	err = getter_msg_process(link_ipc, &msg, sizeof(msg));
	if (err < 0) {
		return err;
	}

	return 0;
}

static union log_msg_generic *link_remote_get_msg(const struct log_link *link)
{
	const struct log_link_ipc *link_ipc = link->ctx;
	struct log_link_ipc_data *data = link_ipc->data;

	if (data->msg) {
		return data->msg;
	}

	if (IS_ENABLED(CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD) && link_ipc->msg_buffer) {
		struct log_link_ipc_msg_buffer *msg_buffer = spsc_peek(link_ipc->msg_buffer);

		if (msg_buffer == NULL) {
			return NULL;
		}

		struct log_ipc_service_msg *msg = msg_buffer->buffer;

		data->msg = (union log_msg_generic *)&msg->data.log_msg.data[data->current_offset];
	} else {
		data->msg = (union log_msg_generic *)mpsc_pbuf_claim(link_ipc->buffer);
	}

	return data->msg;
}

static void link_remote_put_msg(const struct log_link *link, union log_msg_generic *msg)
{
	const struct log_link_ipc *link_ipc = link->ctx;
	struct log_link_ipc_data *data = link_ipc->data;

	if (data->msg == NULL) {
		return;
	}

	if (!IS_ENABLED(CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD) && link_ipc->buffer) {
		mpsc_pbuf_free(link_ipc->buffer, (const union mpsc_pbuf_generic *)msg);
		data->msg = NULL;
		return;
	}

	size_t size = log_msg_generic_get_wlen((union mpsc_pbuf_generic *)data->msg) *
		      sizeof(uint32_t);
	struct log_link_ipc_msg_buffer *msg_buffer = spsc_peek(link_ipc->msg_buffer);

	data->current_offset += size;

	__ASSERT_NO_MSG(data->current_offset <= msg_buffer->size);

	/* All messages from the current IPC RX buffer has been processed. It can be released. */
	if (data->current_offset == msg_buffer->size) {
		int err = ipc_service_release_rx_buffer(&data->ept, msg_buffer->buffer);

		(void)err;
		__ASSERT_NO_MSG(err >= 0);
		data->current_offset = 0;
		spsc_consume(link_ipc->msg_buffer);
		spsc_release(link_ipc->msg_buffer);
	}

	data->msg = NULL;
}

struct log_link_api log_link_ipc_api = {
	.initiate = link_ipc_initiate,
	.activate = link_remote_activate,
	.get_source_name = link_remote_get_source_name,
	.get_levels = link_remote_get_levels,
	.set_runtime_level = link_remote_set_runtime_level,
	.get_msg = link_remote_get_msg,
	.put_msg = link_remote_put_msg,
};

void log_link_ipc_bound_cb(void *priv)
{
}

static void on_msg_received(const struct log_link_ipc *link_ipc,
				struct log_ipc_service_msg *msg,
				size_t len)
{
	struct log_link_ipc_data *data = link_ipc->data;

	if (!IS_ENABLED(CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD) && link_ipc->buffer) {
		struct log_msg *dst_msg;
		k_timeout_t timeout = (CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS == -1)
			? K_FOREVER
			: K_MSEC(CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS);

		len -= offsetof(struct log_ipc_service_msg, data.log_msg.data);

		/* put msgs into mpsc buffer */
		dst_msg = (struct log_msg *)mpsc_pbuf_alloc(link_ipc->buffer,
							    len / sizeof(uint32_t),
							    timeout);

		if (dst_msg == NULL) {
			msg_dropped(link_ipc, msg->status);
			return;
		}

		memcpy(dst_msg, &msg->data.log_msg.data, len);
		mpsc_pbuf_commit(link_ipc->buffer, (union mpsc_pbuf_generic *)dst_msg);
	} else {
		struct log_link_ipc_msg_buffer *msg_buffer = spsc_acquire(link_ipc->msg_buffer);
		int err;

		if (msg_buffer == NULL) {
			msg_dropped(link_ipc, msg->status);
			return;
		}

		msg_buffer->buffer = msg;
		msg_buffer->size = len - offsetof(struct log_ipc_service_msg, data.log_msg.data);

		err = ipc_service_hold_rx_buffer(&data->ept, msg);
		(void)err;
		__ASSERT_NO_MSG(err >= 0);

		spsc_produce(link_ipc->msg_buffer);
	}

	/* Notify logging thread that new messages are available. */
	z_log_msg_remote_notify(msg->status);
}

static void handle_get_name_response(struct log_link_ipc_data *data, char *name, size_t len)
{

	size_t slen = MIN(len - 1, data->rsp.name.len - 1);

	data->rsp.name.len = len - 1;
	memcpy(data->rsp.name.rsp, name, slen);
	data->rsp.name.rsp[slen] = '\0';
}

void log_link_ipc_received_cb(const void *buffer, size_t len, void *priv)
{
	struct log_link_ipc *link_ipc = priv;
	struct log_link_ipc_data *data = link_ipc->data;
	struct log_ipc_service_msg *msg = (struct log_ipc_service_msg *)buffer;

	/* First handle message with logging messages (the most common case). */
	if (msg->id == Z_LOG_IPC_SERVICE_ID_MSG) {
		on_msg_received(link_ipc, msg, len);
		return;
	}

	/* Now handle other messages. Typically, data getters. */
	if (msg->status != Z_LOG_IPC_SERVICE_STATUS_OK) {
		data->status = -EIO;
		goto exit;
	} else {
		data->status = 0;
	}

	switch (msg->id) {
	case Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME:
	{
		size_t str_len = len - offsetof(struct log_ipc_service_msg, data.source_name.name);

		handle_get_name_response(data, msg->data.source_name.name, str_len);
		break;
	}
	case Z_LOG_IPC_SERVICE_ID_GET_LEVELS:
		data->rsp.levels.level = msg->data.levels.level;
		data->rsp.levels.runtime_level = msg->data.levels.runtime_level;
		break;
	case Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL:
		data->rsp.set_runtime_level.level = msg->data.set_rt_level.runtime_level;
		break;
	case Z_LOG_IPC_SERVICE_ID_DROPPED:
		msg_dropped(link_ipc, msg->data.dropped.dropped);
		return;
	case Z_LOG_IPC_SERVICE_ID_READY:
		link_ipc->link->ctrl_blk->sources =
			(struct log_source_const_data *)msg->data.ready.source_addr;
		link_ipc->link->ctrl_blk->log_str_ptr =
			(const char **)(msg->data.ready.log_str_ptr);
		link_ipc->link->ctrl_blk->source_cnt = msg->data.ready.source_count;
		data->ready = true;
		break;
	default:
		__ASSERT(0, "Unexpected message");
		return;
	}

exit:
	k_sem_give(&data->rdy_sem);
}

#ifdef IPC_NODE
/* If there is only a single IPC node we can create it here as the default. */
LOG_LINK_IPC_DEFINE(cpu1, IPC_NODE,
	COND_CODE_1(CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD,
		(0), (CONFIG_LOG_LINK_IPC_SERVICE_BUFFER_SIZE)), true);
#endif
