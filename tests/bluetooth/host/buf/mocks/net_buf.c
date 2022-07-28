/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/buf.h>
#include "net_buf.h"
#include "buf_help_utils.h"


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

/* struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool, k_timeout_t timeout) mock */
DEFINE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_alloc_fixed, struct net_buf_pool *, k_timeout_t);

/* void net_buf_simple_reserve(struct net_buf_simple *buf, size_t reserve) mock */
DEFINE_FAKE_VOID_FUNC(net_buf_simple_reserve, struct net_buf_simple *, size_t);

/* struct net_buf *net_buf_ref(struct net_buf *buf) mock */
DEFINE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_ref, struct net_buf *);
