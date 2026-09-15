/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include "mock_log_link.h"

static int initiate(const struct log_link *link, struct log_link_config *configa)
{
	link->ctrl_blk->source_cnt = 1;

	return 0;
}

static int activate(const struct log_link *link)
{
	return 0;
}

static int get_source_name(const struct log_link *link,
			uint16_t source_id, char *buf, size_t *length)
{
	if (length) {
		*length = 0;
	}

	return 0;
}

static int get_levels(const struct log_link *link,
		      uint16_t source_id, uint8_t *level, uint8_t *runtime_level)
{
	if (level) {
		*level = LOG_LEVEL_INF;
	}
	if (runtime_level) {
		*runtime_level = LOG_LEVEL_INF;
	}

	return 0;
}

static int set_runtime_level(const struct log_link *link, uint16_t source_id, uint8_t level)
{
	return 0;
}

void mock_log_link_add_message(const struct log_link *link, struct log_msg *msg)
{
	size_t msg_len = log_msg_get_total_wlen(msg->hdr.desc) * sizeof(uint32_t);
	struct mock_log_link_ctx *ctx = (struct mock_log_link_ctx *)link->ctx;
	void *dst = &ctx->buf[ctx->buf_index];

	memcpy(dst, msg, msg_len);
	zassert_true(ctx->buf_index + msg_len <= sizeof(ctx->buf), "Buffer overflow");
	zassert_true(ctx->msg_count < ARRAY_SIZE(ctx->msgs), "Message buffer overflow");
	ctx->buf_index += msg_len;
	ctx->msgs[ctx->msg_count++] = (struct log_msg *)dst;

	z_log_msg_remote_notify(1);
}

void mock_log_link_reset(const struct log_link *link)
{
	struct mock_log_link_ctx *ctx = (struct mock_log_link_ctx *)link->ctx;

	ctx->buf_index = 0;
	ctx->msg_count = 0;
	ctx->msg_read_index = 0;
}

static union log_msg_generic *get_msg(const struct log_link *link)
{
	struct mock_log_link_ctx *ctx = (struct mock_log_link_ctx *)link->ctx;

	if (ctx->msg_read_index < ctx->msg_count) {
		return (union log_msg_generic *)ctx->msgs[ctx->msg_read_index];
	}

	return NULL;
}

static void put_msg(const struct log_link *link, union log_msg_generic *msg)
{
	struct mock_log_link_ctx *ctx = (struct mock_log_link_ctx *)link->ctx;

	ctx->msg_read_index++;
}

struct log_link_api mock_log_link_api = {
	.initiate = initiate,
	.activate = activate,
	.get_source_name = get_source_name,
	.get_levels = get_levels,
	.set_runtime_level = set_runtime_level,
	.get_msg = get_msg,
	.put_msg = put_msg,
};
