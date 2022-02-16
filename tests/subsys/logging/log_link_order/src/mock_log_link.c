/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr/logging/log_link.h>

static int initiate(const struct log_link *link, struct log_link_config *configa)
{
	link->ctrl_blk->domain_cnt = 1;
	link->ctrl_blk->source_cnt[0] = 1;

	return 0;
}

static int activate(const struct log_link *link)
{
	return 0;
}

static int get_domain_name(const struct log_link *link, uint32_t domain_id,
			char *buf, size_t *length)
{
	return 0;
}

static int get_source_name(const struct log_link *link, uint32_t domain_id,
			uint16_t source_id, char *buf, size_t *length)
{
	return 0;
}

static int get_levels(const struct log_link *link, uint32_t domain_id,
			uint16_t source_id, uint8_t *level, uint8_t *runtime_level)
{
	return 0;
}

static int set_runtime_level(const struct log_link *link, uint32_t domain_id,
				uint16_t source_id, uint8_t level)
{
	return 0;
}

struct log_link_api mock_log_link_api = {
	.initiate = initiate,
	.activate = activate,
	.get_domain_name = get_domain_name,
	.get_source_name = get_source_name,
	.get_levels = get_levels,
	.set_runtime_level = set_runtime_level,
};
