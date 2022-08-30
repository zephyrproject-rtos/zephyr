/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/buf.h>
#include <mocks/net_buf.h>

DEFINE_FFF_GLOBALS;

static uint8_t *fixed_data_alloc(struct net_buf *buf, size_t *size,
			      k_timeout_t timeout)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return NULL;
}

static void fixed_data_unref(struct net_buf *buf, uint8_t *data)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
}

const struct net_buf_data_cb net_buf_fixed_cb = {
	.alloc = fixed_data_alloc,
	.unref = fixed_data_unref,
};

DEFINE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_alloc_fixed, struct net_buf_pool *, k_timeout_t);
DEFINE_FAKE_VOID_FUNC(net_buf_simple_reserve, struct net_buf_simple *, size_t);
DEFINE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_ref, struct net_buf *);
