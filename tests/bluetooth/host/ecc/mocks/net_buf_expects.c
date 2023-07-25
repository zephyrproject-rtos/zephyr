/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"

#include <zephyr/bluetooth/buf.h>
#include <zephyr/kernel.h>

void expect_single_call_net_buf_simple_add(struct net_buf_simple *buf, size_t len)
{
	const char *func_name = "net_buf_simple_add";

	zassert_equal(net_buf_simple_add_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal(net_buf_simple_add_fake.arg0_val, buf,
		      "'%s()' was called with incorrect '%s' value", func_name, "buf");
	zassert_equal(net_buf_simple_add_fake.arg1_val, len,
		      "'%s()' was called with incorrect '%s' value", func_name, "len");
}

void expect_not_called_net_buf_simple_add(void)
{
	const char *func_name = "net_buf_simple_add";

	zassert_equal(net_buf_simple_add_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
