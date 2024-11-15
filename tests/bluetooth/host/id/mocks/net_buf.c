/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/buf.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#include <mocks/net_buf.h>

DEFINE_FAKE_VOID_FUNC(net_buf_unref, struct net_buf *);
DEFINE_FAKE_VALUE_FUNC(void *, net_buf_simple_add, struct net_buf_simple *, size_t);
DEFINE_FAKE_VALUE_FUNC(uint8_t *, net_buf_simple_add_u8, struct net_buf_simple *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(void *, net_buf_simple_add_mem, struct net_buf_simple *, const void *,
		       size_t);
