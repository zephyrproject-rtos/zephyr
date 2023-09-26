/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define NET_BUF_FFF_FAKES_LIST(FAKE) FAKE(net_buf_simple_add)

DECLARE_FAKE_VALUE_FUNC(void *, net_buf_simple_add, struct net_buf_simple *, size_t);
