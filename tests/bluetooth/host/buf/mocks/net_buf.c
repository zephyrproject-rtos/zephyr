/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/buf.h>
#include "buf_help_utils.h"


static uint8_t *fixed_data_alloc(struct net_buf *buf, size_t *size,
			      k_timeout_t timeout)
{
	ztest_check_expected_value(buf);
	ztest_check_expected_value(size);
	net_buf_validate_timeout_value_mock(timeout.ticks);
	return (uint8_t *)ztest_get_return_value_ptr();
}

static void fixed_data_unref(struct net_buf *buf, uint8_t *data)
{
	/* Nothing needed for fixed-size data pools */
	ztest_check_expected_value(buf);
	ztest_check_expected_value(data);
}

const struct net_buf_data_cb net_buf_fixed_cb = {
	.alloc = fixed_data_alloc,
	.unref = fixed_data_unref,
};

void net_buf_validate_timeout_value_mock(uint32_t value)
{
	ztest_check_expected_value(value);
}

struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool, k_timeout_t timeout)
{
	ztest_check_expected_value(pool);
	net_buf_validate_timeout_value_mock(timeout.ticks);
	return (struct net_buf *)ztest_get_return_value_ptr();
}

void net_buf_simple_reserve(struct net_buf_simple *buf, size_t reserve)
{
	ztest_check_expected_value(buf);
	ztest_check_expected_value(reserve);
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	ztest_check_expected_value(buf);
	return buf;
}
