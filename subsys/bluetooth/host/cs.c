/* cs.c - Bluetooth Channel Sounding handling */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>

#include "conn_internal.h"

#if defined(CONFIG_BT_CHANNEL_SOUNDING)
int bt_cs_set_default_settings(struct bt_conn *conn,
			       const struct bt_cs_set_default_settings_param *params)
{
	struct bt_hci_cp_le_cs_set_default_settings *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CS_SET_DEFAULT_SETTINGS, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->max_tx_power = params->max_tx_power;
	cp->cs_sync_antenna_selection = params->cs_sync_antenna_selection;
	cp->role_enable = 0;

	if (params->enable_initiator_role) {
		cp->role_enable |= BT_HCI_OP_LE_CS_INITIATOR_ROLE_MASK;
	}

	if (params->enable_reflector_role) {
		cp->role_enable |= BT_HCI_OP_LE_CS_REFLECTOR_ROLE_MASK;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CS_SET_DEFAULT_SETTINGS, buf, NULL);
}
#endif /* CONFIG_BT_CHANNEL_SOUNDING */
