/*
 * Copyright (c) 2026 Kent Hauser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test connection meta data: records that the vendor initializer ran on the connection. */
struct lll_conn_meta {
	uint8_t inited;
};
