/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_bt_sample);

static struct k_work advertise_work;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMP_BT_SVC_UUID_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void advertise(struct k_work *work)
{
	int rc;

	rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (rc) {
		LOG_ERR("Advertising failed to start (rc %d)", rc);
		return;
	}

	LOG_INF("Advertising successfully started");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		k_work_submit(&advertise_work);
	} else {
		struct bt_conn_info info;
		char peer[BT_ADDR_LE_STR_LEN] = "unknown";
		char local[BT_ADDR_LE_STR_LEN] = "unknown";
		int32_t rc;

		rc = bt_conn_get_info(conn, &info);
		if (rc != 0) {
			LOG_WRN("Failed to get connection info (rc %d)", rc);
		} else {
			bt_addr_le_to_str(info.le.remote, peer, sizeof(peer));
			bt_addr_le_to_str(info.le.local, local, sizeof(local));
		}

		LOG_INF("Connected, peer %s, local %s", peer, local);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;
	char peer[BT_ADDR_LE_STR_LEN] = "unknown";
	int32_t rc;

	rc = bt_conn_get_info(conn, &info);
	if (rc != 0) {
		LOG_WRN("Failed to get connection info (rc %d)", rc);
	} else {
		bt_addr_le_to_str(info.le.remote, peer, sizeof(peer));
	}

	LOG_INF("Disconnected, reason 0x%02x %s, peer %s", reason,
		bt_hci_err_to_str(reason), peer);
}

static void on_conn_recycled(void)
{
	k_work_submit(&advertise_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = on_conn_recycled,
};

static void bt_ready(int err)
{
	if (err != 0) {
		LOG_ERR("Bluetooth failed to initialise: %d", err);
	} else {
		k_work_submit(&advertise_work);
	}
}

void start_smp_bluetooth_adverts(void)
{
	int rc;

	k_work_init(&advertise_work, advertise);
	rc = bt_enable(bt_ready);

	if (rc != 0) {
		LOG_ERR("Bluetooth enable failed: %d", rc);
	}
}
