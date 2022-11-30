/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"

#include <zephyr/bluetooth/buf.h>
#include <zephyr/kernel.h>

void expect_single_call_net_buf_unref(struct net_buf *buf)
{
	const char *func_name = "net_buf_unref";

	zassert_equal(net_buf_unref_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal(net_buf_unref_fake.arg0_val, buf,
		      "'%s()' was called with incorrect '%s' value", func_name, "buf");
}

void expect_not_called_net_buf_unref(void)
{
	const char *func_name = "net_buf_unref";

	zassert_equal(net_buf_unref_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
