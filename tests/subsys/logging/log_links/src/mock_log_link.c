/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mock_log_link.h"
#include <string.h>

static int initiate(const struct log_link *link, struct log_link_config *configa)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	link->ctrl_blk->source_cnt = mock->source_cnt;

	return 0;
}

static int activate(const struct log_link *link)
{
	return 0;
}

static int get_source_name(const struct log_link *link, uint16_t source_id,
			char *buf, size_t *length)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	if (source_id >= mock->source_cnt) {
		return -EINVAL;
	}

	strncpy(buf, mock->sources[source_id].source, *length);
	*length = strlen(mock->sources[source_id].source);

	return 0;
}

static int get_levels(const struct log_link *link, uint16_t source_id,
			uint8_t *level, uint8_t *runtime_level)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	*level = mock->sources[source_id].clevel;

	if (runtime_level) {
		*runtime_level = mock->sources[source_id].rlevel;
	}

	return 0;
}

static int set_runtime_level(const struct log_link *link, uint16_t source_id, uint8_t level)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	mock->sources[source_id].rlevel = level;

	return 0;
}

static union log_msg_generic *get_msg(const struct log_link *link)
{
	return NULL;
}

static void put_msg(const struct log_link *link, union log_msg_generic *msg)
{
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
