/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/* List of fakes used by this unit tester */
#define NET_BUF_FFF_FAKES_LIST(FAKE)                                                               \
	FAKE(net_buf_unref)                                                                        \
	FAKE(net_buf_reset)                                                                        \
	FAKE(net_buf_slist_put)                                                                    \
	FAKE(net_buf_alloc_fixed)

DECLARE_FAKE_VOID_FUNC(net_buf_unref, struct net_buf *);
DECLARE_FAKE_VOID_FUNC(net_buf_reset, struct net_buf *);
DECLARE_FAKE_VOID_FUNC(net_buf_slist_put, sys_slist_t *, struct net_buf *);
DECLARE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_alloc_fixed, struct net_buf_pool *, k_timeout_t);
