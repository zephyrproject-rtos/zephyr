/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "l2cap_internal.h"

DEFINE_FAKE_VOID_FUNC(bt_l2cap_init);
DEFINE_FAKE_VOID_FUNC(bt_l2cap_recv, struct bt_conn *, struct net_buf *, bool);
DEFINE_FAKE_VOID_FUNC(bt_l2cap_connected, struct bt_conn *);
DEFINE_FAKE_VALUE_FUNC(int, bt_l2cap_update_conn_param, struct bt_conn *,
		       const struct bt_le_conn_param *);
DEFINE_FAKE_VOID_FUNC(bt_l2cap_disconnected, struct bt_conn *);
DEFINE_FAKE_VALUE_FUNC(struct net_buf *, l2cap_data_pull, struct bt_conn *, size_t, size_t *);
