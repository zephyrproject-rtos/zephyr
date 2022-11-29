/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define NET_BUF_FFF_FAKES_LIST(FAKE)                                                               \
	FAKE(net_buf_unref)                                                                        \
	FAKE(net_buf_simple_add)                                                                   \
	FAKE(net_buf_simple_add_u8)                                                                \
	FAKE(net_buf_simple_add_mem)

DECLARE_FAKE_VOID_FUNC(net_buf_unref, struct net_buf *);
DECLARE_FAKE_VALUE_FUNC(void *, net_buf_simple_add, struct net_buf_simple *, size_t);
DECLARE_FAKE_VALUE_FUNC(uint8_t *, net_buf_simple_add_u8, struct net_buf_simple *, uint8_t);
DECLARE_FAKE_VALUE_FUNC(void *, net_buf_simple_add_mem, struct net_buf_simple *, const void *,
			size_t);
