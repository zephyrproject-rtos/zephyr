/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_LOG_LINK_H__
#define MOCK_LOG_LINK_H__

#include <zephyr/logging/log_link.h>

extern struct log_link_api mock_log_link_api;

struct mock_log_link_ctx {
	uint8_t buf[512];
	struct log_msg *msgs[8];
	int msg_count;
	int msg_read_index;
	int buf_index;
	int ready;
};

void mock_log_link_reset(const struct log_link *link);
void mock_log_link_add_message(const struct log_link *link, struct log_msg *msg);

#define MOCK_LOG_LINK_DEFINE(_n) \
	static struct mock_log_link_ctx _n##_context; \
	LOG_LINK_DEFINE(_n, mock_log_link_api, &_n##_context)

#endif /* MOCK_LOG_LINK_H__ */
