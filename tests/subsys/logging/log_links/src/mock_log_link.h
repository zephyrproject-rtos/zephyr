/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_LOG_LINK_H__
#define MOCK_LOG_LINK_H__

#include <zephyr/logging/log_link.h>

extern struct log_link_api mock_log_link_api;

struct mock_log_link_source {
	const char *source;
	uint8_t clevel;
	uint8_t rlevel;
};

struct mock_log_link {
	uint16_t source_cnt;
	struct mock_log_link_source *sources;
};

#define MOCK_LOG_LINK_DEFINE(_name, _ctx) \
	LOG_LINK_DEFINE(_name, mock_log_link_api, _ctx)

#endif /* MOCK_LOG_LINK_H__ */
