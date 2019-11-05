/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <string.h>
#include <logging/log_link.h>
#include <logging/log.h>
#include "log_link_open_amp.h"
#include "log_multidomain_internal.h"

LOG_MODULE_REGISTER(log_link_oamp);

struct log_link_oamp_source_count {
	u16_t *count;
};

struct log_link_oamp_source_domain_name {
	u8_t *buf;
	u32_t *len;
};

struct log_link_oamp_log_level {
	u8_t *level;
};

struct log_link_oamp_timestamp {
	u32_t timestamp;
};

union response {
	struct log_link_oamp_source_count source_count;
	struct log_link_oamp_source_domain_name source_domain_name;
	struct log_link_oamp_log_level level;
	struct log_link_oamp_timestamp timestamp;
};

static K_SEM_DEFINE(log_link_oamp_sync_sem, 0, 1);
static K_MUTEX_DEFINE(log_link_oamp_access_mtx);

struct log_link_oamp_ctrl_blk {
	union response rsp;
	struct k_sem *sync_sem;
	struct k_mutex *access_mtx;
	struct log_msg *msg;
	int rem_msg_len;
	u16_t msg_len;
};

static struct log_link_oamp_ctrl_blk ctrl_blk = {
	.sync_sem = &log_link_oamp_sync_sem,
	.access_mtx = &log_link_oamp_access_mtx
};

static void get_source_domain_name_rsp(struct log_link_oamp_ctrl_blk *ctrl_blk,
				       u8_t *data, size_t len)
{
	if (ctrl_blk->rsp.source_domain_name.buf) {
		memcpy(ctrl_blk->rsp.source_domain_name.buf, data, len);
		ctrl_blk->rsp.source_domain_name.buf[len] = 0;
	}

	if (ctrl_blk->rsp.source_domain_name.len) {
		*ctrl_blk->rsp.source_domain_name.len = len;
	}
}

static void get_source_count_rsp(struct log_link_oamp_ctrl_blk *ctrl_blk,
				 u8_t *data, size_t len)
{
	if (ctrl_blk->rsp.source_count.count) {
		memcpy(ctrl_blk->rsp.source_count.count,  data, sizeof(u16_t));
	}
}

static void get_level(struct log_link_oamp_ctrl_blk *ctrl_blk,
		      u8_t *data, size_t len)
{
	if (ctrl_blk->rsp.level.level) {
		*ctrl_blk->rsp.level.level = *data;
	}
}

static void get_timestamp(struct log_link_oamp_ctrl_blk *ctrl_blk,
		      u8_t *data, size_t len)
{
	ctrl_blk->rsp.timestamp.timestamp = *(u32_t *)data;
}


static void receive_msg_done(struct log_link_oamp_ctrl_blk *ctrl_blk)
{
	ctrl_blk->msg->hdr.params.hexdump.length = ctrl_blk->msg_len;
	z_log_msg_enqueue(ctrl_blk->msg);
	ctrl_blk->msg = NULL;
}

static void abort_msg(struct log_link_oamp_ctrl_blk *ctrl_blk)
{
	log_msg_put(ctrl_blk->msg);
	ctrl_blk->msg = NULL;
}

static void receive_msg_head(const struct log_link *link,
			     struct log_msg *msg)
{
	__ASSERT(ctrl_blk->msg == NULL, "Another message already in progress.");
	struct log_link_oamp_ctrl_blk *ctrl_blk =
				(struct log_link_oamp_ctrl_blk *)link->ctx;

	ctrl_blk->msg = (struct log_msg *)log_msg_chunk_alloc();

	if (ctrl_blk->msg == NULL) {
		/* Failed to allocate. Whole message will be discarded. */
		return;
	}

	memcpy(ctrl_blk->msg, msg, sizeof(struct log_msg));
	log_link_msg_prepare(link, ctrl_blk->msg);
	ctrl_blk->rem_msg_len = msg->hdr.params.hexdump.length;
	ctrl_blk->msg_len =  msg->hdr.params.hexdump.length;
	if (ctrl_blk->msg_len <= LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK) {
		receive_msg_done(ctrl_blk);
	} else {
		ctrl_blk->rem_msg_len -= LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
		ctrl_blk->msg->payload.ext.next = NULL;
		ctrl_blk->msg->hdr.params.hexdump.length =
				LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
	}
}

static void receive_msg_cont(struct log_link_oamp_ctrl_blk *ctrl_blk,
			     struct log_msg_cont *in_cont)
{
	struct log_msg_cont **cont;

	if (ctrl_blk->msg == NULL) {
		/* If message was not allocated, drop any continuation. */
		return;
	}

	cont = &ctrl_blk->msg->payload.ext.next;
	while (*cont != NULL) {
		cont = &((*cont)->next);
	}

	*cont = (struct log_msg_cont *)log_msg_chunk_alloc();
	if (*cont == NULL) {
		abort_msg(ctrl_blk);
	}

	memcpy(*cont, in_cont, sizeof(struct log_msg_cont));
	(*cont)->next = NULL;
	ctrl_blk->rem_msg_len -= HEXDUMP_BYTES_CONT_MSG;
	ctrl_blk->msg->hdr.params.hexdump.length += HEXDUMP_BYTES_CONT_MSG;
	if (ctrl_blk->rem_msg_len <= 0) {
		receive_msg_done(ctrl_blk);
	}
}

static void rx_clbk(const struct log_link *link,
		    u8_t *data, size_t len)
{
	struct log_link_oamp_ctrl_blk *ctrl_blk =
			(struct log_link_oamp_ctrl_blk *)link->ctx;
	u8_t id = data[0];

	data++;
	len--;

	switch(id) {
	case LOG_MULDOMAIN_DOMAIN_NAME_GET:
	case LOG_MULDOMAIN_SOURCE_NAME_GET:
		get_source_domain_name_rsp(ctrl_blk, data, len);
		break;
	case LOG_MULDOMAIN_SOURCE_COUNT_GET:
		get_source_count_rsp(ctrl_blk, data, len);
		break;
	case LOG_MULDOMAIN_COMPILED_LEVEL_GET:
	case LOG_MULDOMAIN_RUNTIME_LEVEL_GET:
		get_level(ctrl_blk, data, len);
		break;
	case LOG_MULDOMAIN_RUNTIME_LEVEL_SET:
		break;
	case LOG_MULDOMAIN_TIMESTAMP_GET:
		get_timestamp(ctrl_blk, data, len);
		break;
	case LOG_MULDOMAIN_MSG_HEAD:
	{
		struct log_msg *msg = (struct log_msg *)(--data);
		__ASSERT((len + 1) == sizeof(struct log_msg),
			"Unexpected message size");
		receive_msg_head(link, msg);
		return;
	}
	case LOG_MULDOMAIN_MSG_CONT:
	{
		struct log_msg_cont *cont = (struct log_msg_cont *)(--data);
		__ASSERT((len + 1) == sizeof(struct log_msg_cont),
			"Unexpected message size");
		receive_msg_cont(ctrl_blk, cont);
		return;
	}
	default:
		break;
	}

	k_sem_give(ctrl_blk->sync_sem);
}

static void oamp_rx_clbk(u8_t *data, size_t len);
static u32_t timestamp_get(const struct log_link *link);

static int get_timestamp_offset(const struct log_link *link)
{
	u32_t remote_timestamp;
	u32_t local_timestamp;
	u32_t op_time;
	int offset;

	local_timestamp = z_log_get_timestamp();
	remote_timestamp = timestamp_get(link);
	op_time = z_log_get_timestamp() - local_timestamp;
	offset = (local_timestamp + op_time/2) - remote_timestamp;

	return offset;
}

static int init(const struct log_link *link, log_link_callback_t callback)
{
	int err;

	err = log_link_open_amp_init(oamp_rx_clbk);
	link->ctrl_blk->domain_cnt = 1;
	link->ctrl_blk->timestamp_offset = get_timestamp_offset(link);

	return 0;
}

static int req_handle(const struct log_link *link, u8_t *msg, int msg_len,
		      union response *new_response)
{
	int err;
	struct log_link_oamp_ctrl_blk *ctrl_blk =
				(struct log_link_oamp_ctrl_blk *)link->ctx;

	k_mutex_lock(ctrl_blk->access_mtx, K_FOREVER);

	if (new_response) {
		memcpy(&ctrl_blk->rsp, new_response, sizeof(union response));
	}

	log_link_open_amp_send(msg, msg_len);
	err = k_sem_take(ctrl_blk->sync_sem, K_MSEC(20));

	if (new_response) {
		memcpy(new_response, &ctrl_blk->rsp, sizeof(union response));
	}

	k_mutex_unlock(ctrl_blk->access_mtx);

	return err;
}

static int get_domain_name(const struct log_link *link, u8_t domain_id,
			   char *buf, u32_t *length)
{
	union response rsp = {
		.source_domain_name = { .buf = buf, .len = length }
	};
	u8_t msg[] = {LOG_MULDOMAIN_DOMAIN_NAME_GET, domain_id};

	return req_handle(link, msg, sizeof(msg), &rsp);
}

static u16_t get_source_count(const struct log_link *link, u8_t domain_id)
{
	int err;
	u8_t msg[] = {LOG_MULDOMAIN_SOURCE_COUNT_GET, domain_id};
	u16_t count = 0;
	union response rsp = { .source_count = { .count = &count }};

	err = req_handle(link, msg, sizeof(msg), &rsp);
	if (err) {
		LOG_ERR("Failed to read source count");
		return 0;
	}

	return count;
}

static int get_source_name(const struct log_link *link, u8_t domain_id,
			   u16_t source_id, char *buf, u32_t length)
{
	union response rsp = {
		.source_domain_name = { .buf = buf, .len = NULL }
	};
	u8_t msg[] = {
		LOG_MULDOMAIN_SOURCE_NAME_GET, domain_id,
		(u8_t)source_id, (u8_t)(source_id >> 8)
	};

	return req_handle(link, msg, sizeof(msg), &rsp);
}

static int get_compiled_level(const struct log_link *link, u8_t domain_id,
			      u16_t source_id, u8_t *level)
{
	union response rsp = {
		.level = { .level = level }
	};
	u8_t msg[] = {
		LOG_MULDOMAIN_COMPILED_LEVEL_GET, domain_id,
		(u8_t)source_id, (u8_t)(source_id >> 8)
	};

	return req_handle(link, msg, sizeof(msg), &rsp);
}

static int get_runtime_level(const struct log_link *link, u8_t domain_id,
			      u16_t source_id, u8_t *level)
{
	union response rsp = {
		.level = { .level = level }
	};
	u8_t msg[] = {
		LOG_MULDOMAIN_RUNTIME_LEVEL_GET, domain_id,
		(u8_t)source_id, (u8_t)(source_id >> 8)
	};

	return req_handle(link, msg, sizeof(msg), &rsp);
}

static int set_runtime_level(const struct log_link *link, u8_t domain_id,
			      u16_t source_id, u8_t level)
{
	u8_t msg[] = {
		LOG_MULDOMAIN_RUNTIME_LEVEL_SET, domain_id,
		(u8_t)source_id, (u8_t)(source_id >> 8), level
	};

	return req_handle(link, msg, sizeof(msg), NULL);
}

static u32_t timestamp_get(const struct log_link *link)
{
	u8_t msg[] = {LOG_MULDOMAIN_TIMESTAMP_GET};
	union response rsp;
	int err;

	err = req_handle(link, msg, sizeof(msg), &rsp);
	if (err != 0) {
		return 0;
	}

	return rsp.timestamp.timestamp;
}

static const struct log_link_api api = {
	.init = init,
	.get_domain_name = get_domain_name,
	.get_source_count = get_source_count,
	.get_source_name = get_source_name,
	.get_compiled_level = get_compiled_level,
	.get_runtime_level = get_runtime_level,
	.set_runtime_level = set_runtime_level,
};

LOG_LINK_DEF(log_link_oamp, api, &ctrl_blk);

static void oamp_rx_clbk(u8_t *data, size_t len)
{
	rx_clbk(&log_link_oamp, data, len);
}
