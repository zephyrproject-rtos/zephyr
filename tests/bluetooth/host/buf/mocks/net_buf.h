/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define NET_BUF_FFF_FAKES_LIST(FAKE)   \
		FAKE(net_buf_alloc_fixed)      \
		FAKE(net_buf_simple_reserve)   \
		FAKE(net_buf_ref)

DECLARE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_alloc_fixed, struct net_buf_pool *, k_timeout_t);
DECLARE_FAKE_VOID_FUNC(net_buf_simple_reserve, struct net_buf_simple *, size_t);
DECLARE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_ref, struct net_buf *);
