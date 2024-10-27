/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/addr.h>

/* List of fakes used by this unit tester */
#define L2CAP_INTERNAL_MOCKS_FFF_FAKES_LIST(FAKE)                                                  \
	FAKE(bt_l2cap_init)                                                                        \
	FAKE(bt_l2cap_recv)                                                                        \
	FAKE(bt_l2cap_connected)                                                                   \
	FAKE(bt_l2cap_update_conn_param)                                                           \
	FAKE(bt_l2cap_disconnected)                                                                \
	FAKE(l2cap_data_pull)

DECLARE_FAKE_VOID_FUNC(bt_l2cap_init);
DECLARE_FAKE_VOID_FUNC(bt_l2cap_recv, struct bt_conn *, struct net_buf *, bool);
DECLARE_FAKE_VOID_FUNC(bt_l2cap_connected, struct bt_conn *);
DECLARE_FAKE_VALUE_FUNC(int, bt_l2cap_update_conn_param, struct bt_conn *,
			const struct bt_le_conn_param *);
DECLARE_FAKE_VOID_FUNC(bt_l2cap_disconnected, struct bt_conn *);
DECLARE_FAKE_VALUE_FUNC(struct net_buf *, l2cap_data_pull, struct bt_conn *, size_t, size_t *);
