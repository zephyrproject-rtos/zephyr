/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mock_log_link.h"
#include <string.h>

static int init(const struct log_link *link, log_link_callback_t callback)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	link->ctrl_blk->domain_cnt = mock->domain_cnt;

	return 0;
}

static u16_t get_source_count(const struct log_link *link, u8_t domain_id)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	return mock->domains[domain_id]->source_cnt;
}

static int get_domain_name(const struct log_link *link, u8_t domain_id,
			char *buf, u32_t *length)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	*length = strlen(mock->domains[domain_id]->name);

	if (buf) {
		strncpy(buf, mock->domains[domain_id]->name, *length);
	}

	return 0;
}

static int get_source_name(const struct log_link *link, u8_t domain_id,
			u16_t source_id, char *buf, u32_t length)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	strncpy(buf, mock->domains[domain_id]->sources[source_id].source,
			length);

	return 0;
}

static int get_compiled_level(const struct log_link *link, u8_t domain_id,
			u16_t source_id, u8_t *level)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	*level = mock->domains[domain_id]->sources[source_id].clevel;

	return 0;
}

static int set_runtime_level(const struct log_link *link, u8_t domain_id,
				u16_t source_id, u8_t level)
{
	struct mock_log_link *mock = (struct mock_log_link *)link->ctx;

	mock->domains[domain_id]->sources[source_id].rlevel = level;

	return 0;
}

struct log_link_api mock_log_link_api = {
	.init = init,
	.get_source_count = get_source_count,
	.get_domain_name = get_domain_name,
	.get_source_name = get_source_name,
	.get_compiled_level = get_compiled_level,
	.set_runtime_level = set_runtime_level,
};
