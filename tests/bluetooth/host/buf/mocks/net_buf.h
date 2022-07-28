/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/zephyr.h>

DEFINE_FFF_GLOBALS;

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)         \
	FAKE(net_buf_alloc_fixed)          \
	FAKE(net_buf_simple_reserve)       \
	FAKE(net_buf_ref)

/* struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool, k_timeout_t timeout) mock */
DECLARE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_alloc_fixed, struct net_buf_pool *, k_timeout_t);

/* void net_buf_simple_reserve(struct net_buf_simple *buf, size_t reserve) mock */
DECLARE_FAKE_VOID_FUNC(net_buf_simple_reserve, struct net_buf_simple *, size_t);

/* struct net_buf *net_buf_ref(struct net_buf *buf) mock */
DECLARE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_ref, struct net_buf *);
