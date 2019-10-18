/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_LOG_LINK_H__
#define MOCK_LOG_LINK_H__

#include <logging/log_link.h>

struct mock_log_link_source {
	const char *source;
	u8_t clevel;
	u8_t rlevel;
};
struct mock_log_link_domain {
	u16_t source_cnt;
	struct mock_log_link_source *sources;
	const char *name;
};

struct mock_log_link {
	u8_t domain_cnt;
	struct mock_log_link_domain **domains;
};

#endif /* MOCK_LOG_LINK_H__ */
