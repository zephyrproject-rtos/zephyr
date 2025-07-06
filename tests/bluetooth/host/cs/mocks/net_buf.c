/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#include "mocks/net_buf.h"

const struct net_buf_data_cb net_buf_fixed_cb;

DEFINE_FAKE_VOID_FUNC(net_buf_unref, struct net_buf *);
DEFINE_FAKE_VOID_FUNC(net_buf_reset, struct net_buf *);
DEFINE_FAKE_VOID_FUNC(net_buf_slist_put, sys_slist_t *, struct net_buf *);
DEFINE_FAKE_VALUE_FUNC(struct net_buf *, net_buf_alloc_fixed, struct net_buf_pool *, k_timeout_t);
