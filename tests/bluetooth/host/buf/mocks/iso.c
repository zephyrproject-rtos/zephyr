/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/iso.h>

/* This file doesn't contain mocks, but rather fakes of the necessary parts of
 * subsys/bluetooth/host/iso.c. The API implementations that are copied here
 * should be kept in sync with the original.
 */

#if defined(CONFIG_BT_ISO_UNICAST) || defined(CONFIG_BT_ISO_SYNC_RECEIVER)
NET_BUF_POOL_FIXED_DEFINE(iso_rx_pool, CONFIG_BT_ISO_RX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_RX_MTU), 8, NULL);

struct net_buf *bt_iso_get_rx(k_timeout_t timeout)
{
	struct net_buf *buf = net_buf_alloc(&iso_rx_pool, timeout);

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, BT_BUF_ISO_IN);
	}

	return buf;
}

struct net_buf_pool *bt_buf_get_iso_rx_pool(void)
{
	return &iso_rx_pool;
}
#endif
