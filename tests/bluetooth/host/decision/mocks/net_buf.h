/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/* List of fakes used by this unit tester */
#define NET_BUF_FFF_FAKES_LIST(FAKE) FAKE(net_buf_unref)

DECLARE_FAKE_VOID_FUNC(net_buf_unref, struct net_buf *);
