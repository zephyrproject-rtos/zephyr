/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>

#include <host/hci_core.h>

#include "hci_core.h"

DEFINE_FAKE_VALUE_FUNC(struct net_buf *, bt_hci_cmd_create, uint16_t, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, bt_hci_cmd_send_sync, uint16_t, struct net_buf *, struct net_buf **);
DEFINE_FAKE_VALUE_FUNC(int, bt_hci_le_read_remote_features, struct bt_conn *);
DEFINE_FAKE_VALUE_FUNC(int, bt_hci_disconnect, uint16_t, uint8_t);
DEFINE_FAKE_VALUE_FUNC(bool, bt_le_conn_params_valid, const struct bt_le_conn_param *);
DEFINE_FAKE_VOID_FUNC(bt_tx_irq_raise);
DEFINE_FAKE_VALUE_FUNC(int, bt_send, struct net_buf *);
DEFINE_FAKE_VOID_FUNC(bt_send_one_host_num_completed_packets, uint16_t);
DEFINE_FAKE_VOID_FUNC(bt_acl_set_ncp_sent, struct net_buf *, bool);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_create_conn, const struct bt_conn *);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_create_conn_cancel);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_create_conn_synced, const struct bt_conn *,
		       const struct bt_le_ext_adv *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(const bt_addr_le_t *, bt_lookup_id_addr, uint8_t, const bt_addr_le_t *);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_set_phy, struct bt_conn *, uint8_t, uint8_t, uint8_t, uint8_t);

struct bt_dev bt_dev = {
	.manufacturer = 0x1234,
};
