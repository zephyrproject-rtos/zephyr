/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_nxp_sco);

#include <common/bt_str.h>

#include <host/conn_internal.h>
#include <host/classic/sco_internal.h>

#define BT_HCI_SET_VOICE_OVER_HCI 0
#define BT_HCI_SET_VOICE_OVER_PCM_PIN 1

static int bt_hci_set_voice_over_hci(void)
{
	int err;
	struct net_buf *buf;

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	__ASSERT(net_buf_tailroom(buf) >= sizeof(uint8_t), "No space in buffer");

	net_buf_add_u8(buf, BT_HCI_SET_VOICE_OVER_HCI);
	err = bt_hci_cmd_send_sync(BT_OP(BT_OGF_VS, 0x001d), buf, NULL);
	return err;
}

static void bt_nxp_connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		return;
	}

	if (bt_hci_set_voice_over_hci() < 0) {
		LOG_ERR("Fail to set voice over HCI");
	}
}

BT_CONN_CB_DEFINE(hci_nxp_conn_cb) = {
	.connected = bt_nxp_connected,
};
