/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/net/buf.h>

#include "subsys/bluetooth/host/classic/at.h"

#include <zephyr/ztest.h>

static struct at_client at;
static char buffer[140];

NET_BUF_POOL_DEFINE(at_pool, 1, 140, 0, NULL);

static const char example_data[] = "\r\n+ABCD:999\r\n";

int at_handle(struct at_client *hf_at)
{
	uint32_t val;

	zassert_equal(at_get_number(hf_at, &val), 0, "Error getting value");

	zassert_equal(val, 999, "Invalid value parsed");

	return 0;
}

int at_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	err = at_parse_cmd_input(hf_at, buf, "ABCD", at_handle,
				 AT_CMD_TYPE_NORMAL);
	zassert_equal(err, 0, "Error parsing CMD input");

	return 0;
}

ZTEST_SUITE(at_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(at_tests, test_at)
{
	struct net_buf *buf;
	int len;

	at.buf_max_len = 140U;
	at.buf = buffer;

	buf = net_buf_alloc(&at_pool, K_FOREVER);
	zassert_not_null(buf, "Failed to get buffer");

	at_register(&at, at_resp, NULL);
	len = strlen(example_data);

	zassert_true(net_buf_tailroom(buf) >= len,
		    "Allocated buffer is too small");
	net_buf_add_mem(buf, example_data, len);

	zassert_equal(at_parse_input(&at, buf), 0, "Parsing failed");
}
