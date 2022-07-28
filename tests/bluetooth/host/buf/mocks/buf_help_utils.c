/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/bluetooth/buf.h>
#include "net_buf.h"
#include "buf_help_utils.h"

void validate_net_buf_alloc_called_behaviour(struct net_buf_pool *pool, k_timeout_t *timeout)
{
	const char *func_name = "net_buf_alloc_fixed";

	zassert_equal(net_buf_alloc_fixed_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal_ptr(net_buf_alloc_fixed_fake.arg0_val, pool,
			  "'%s()' was called with incorrect '%s' value", func_name, "pool");

	zassert_mem_equal(&net_buf_alloc_fixed_fake.arg1_val, timeout, sizeof(k_timeout_t),
			  "'%s()' was called with incorrect '%s' value", func_name, "timeout");
}

void validate_net_buf_alloc_not_called_behaviour(void)
{
	const char *func_name = "net_buf_alloc_fixed";

	zassert_equal(net_buf_alloc_fixed_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}

void validate_net_buf_reserve_called_behaviour(struct net_buf *buf)
{
	const char *func_name = "net_buf_simple_reserve";

	zassert_equal(net_buf_simple_reserve_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal_ptr(net_buf_simple_reserve_fake.arg0_val, &buf->b,
			  "'%s()' was called with incorrect '%s' value", func_name, "buf");

	zassert_equal(net_buf_simple_reserve_fake.arg1_val, BT_BUF_RESERVE,
		      "'%s()' was called with incorrect '%s' value", func_name, "reserve");
}

void validate_net_buf_reserve_not_called_behaviour(void)
{
	const char *func_name = "net_buf_simple_reserve";

	zassert_equal(net_buf_simple_reserve_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
