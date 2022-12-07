/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_LOG_LINK_H__
#define MOCK_LOG_LINK_H__

#include <zephyr/logging/log_link.h>

struct mock_log_link_source {
	const char *source;
	uint8_t clevel;
	uint8_t rlevel;
};
struct mock_log_link_domain {
	uint16_t source_cnt;
	struct mock_log_link_source *sources;
	const char *name;
};

struct mock_log_link {
	uint8_t domain_cnt;
	struct mock_log_link_domain **domains;
};

#endif /* MOCK_LOG_LINK_H__ */
