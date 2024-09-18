/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util.h>

#define MTU_TEST_SERVICE_TYPE BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f0)

/* Overhead: opcode (u8) + handle (u16) */
#define ATT_NTF_SIZE(payload_len) (1 + 2 + payload_len)

static const struct bt_uuid_128 mtu_test_service = BT_UUID_INIT_128(MTU_TEST_SERVICE_TYPE);

static const struct bt_uuid_128 notify_characteristic_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f3));

static const struct bt_data adv_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, MTU_TEST_SERVICE_TYPE),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static struct bt_conn *default_conn;

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	printk("MTU Test Update: notifications %s\n", notif_enabled ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(mtu_test, BT_GATT_PRIMARY_SERVICE(&mtu_test_service),
		       BT_GATT_CHARACTERISTIC(&notify_characteristic_uuid.uuid, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		return;
	}

	default_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_unref(conn);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

void run_peripheral_sample(uint8_t *notify_data, size_t notify_data_size, uint16_t seconds)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_gatt_cb_register(&gatt_callbacks);

	struct bt_gatt_attr *notify_crch =
		bt_gatt_find_by_uuid(mtu_test.attrs, 0xffff, &notify_characteristic_uuid.uuid);

	bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, adv_ad_data, ARRAY_SIZE(adv_ad_data), NULL, 0);

	bool infinite = seconds == 0;

	for (int i = 0; (i < seconds) || infinite; i++) {
		k_sleep(K_SECONDS(1));
		/* Only send the notification if the UATT MTU supports the required length */
		if (bt_gatt_get_uatt_mtu(default_conn) >= ATT_NTF_SIZE(notify_data_size)) {
			bt_gatt_notify(default_conn, notify_crch, notify_data, notify_data_size);
		} else {
			printk("Skipping notification since UATT MTU is not sufficient."
			       "Required: %d, Actual: %d\n",
			       ATT_NTF_SIZE(notify_data_size),
			       bt_gatt_get_uatt_mtu(default_conn));
		}
	}
}
