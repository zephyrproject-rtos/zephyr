/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/ias.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#define TERM_PRINT(fmt, ...)   printk("\e[39m[Peripheral] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_INFO(fmt, ...)    printk("\e[94m[Peripheral] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_SUCCESS(fmt, ...) printk("\e[92m[Peripheral] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_ERR(fmt, ...)                                                                         \
	printk("\e[91m[Peripheral] %s:%d : " fmt "\e[39m\n", __func__, __LINE__, ##__VA_ARGS__)
#define TERM_WARN(fmt, ...)                                                                        \
	printk("\e[93m[Peripheral] %s:%d : " fmt "\e[39m\n", __func__, __LINE__, ##__VA_ARGS__)

#define NOTIFICATION_DATA_PREFIX     "Counter:"
#define NOTIFICATION_DATA_PREFIX_LEN (sizeof(NOTIFICATION_DATA_PREFIX) - 1)

#define CHARACTERISTIC_DATA_MAX_LEN 260
#define NOTIFICATION_DATA_LEN	    (CONFIG_BT_L2CAP_TX_MTU - 4)
BUILD_ASSERT(NOTIFICATION_DATA_LEN <= CHARACTERISTIC_DATA_MAX_LEN);

#define CENTRAL_SERVICE_UUID_VAL                                                                   \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdea0)

#define CENTRAL_CHARACTERISTIC_UUID_VAL                                                            \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdea1)

#define CENTRAL_SERVICE_UUID	    BT_UUID_DECLARE_128(CENTRAL_SERVICE_UUID_VAL)
#define CENTRAL_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(CENTRAL_CHARACTERISTIC_UUID_VAL)

/* Custom Service Variables */
static struct bt_uuid_128 vnd_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));

static struct bt_uuid_128 vnd_enc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

enum {
	CONN_INFO_SECURITY_LEVEL_UPDATED,
	CONN_INFO_CONN_PARAMS_UPDATED,
	CONN_INFO_LL_DATA_LEN_TX_UPDATED,
	CONN_INFO_LL_DATA_LEN_RX_UPDATED,
	CONN_INFO_MTU_EXCHANGED,
	CONN_INFO_SUBSCRIBED_TO_SERVICE,
	/* Total number of flags - must be at the end of the enum */
	CONN_INFO_NUM_FLAGS,
};

struct active_conn_info {
	ATOMIC_DEFINE(flags, CONN_INFO_NUM_FLAGS);
	struct bt_conn *conn_ref;
	uint32_t notify_counter;
};

static uint8_t simulate_vnd;
static int64_t uptime_ref;
static uint32_t tx_notify_counter;
static struct active_conn_info conn_info;
static uint8_t vnd_value[CHARACTERISTIC_DATA_MAX_LEN];

static void vnd_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	simulate_vnd = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	if (simulate_vnd) {
		tx_notify_counter = 0;
		uptime_ref = k_uptime_get();
	}
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(
	vnd_svc, BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL,
			       NULL),
	BT_GATT_CCC(vnd_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	TERM_INFO("Updated MTU: TX: %d RX: %d bytes", tx, rx);

	if (tx == CONFIG_BT_L2CAP_TX_MTU && rx == CONFIG_BT_L2CAP_TX_MTU) {
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		atomic_set_bit(conn_info.flags, CONN_INFO_MTU_EXCHANGED);
		TERM_SUCCESS("Updating MTU succeeded %s", addr);
	}
}

static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		memset(&conn_info, 0x00, sizeof(struct active_conn_info));
		TERM_ERR("Connection failed (err 0x%02x)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	conn_info.conn_ref = conn;
	TERM_SUCCESS("Connection %p established : %s", conn, addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	memset(&conn_info, 0x00, sizeof(struct active_conn_info));
	TERM_ERR("Disconnected (reason 0x%02x)", reason);
	__ASSERT(reason == BT_HCI_ERR_LOCALHOST_TERM_CONN, "Disconnected (reason 0x%02x)", reason);
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_PRINT("LE conn param req: %s int (0x%04x (~%u ms), 0x%04x (~%u ms)) lat %d to %d",
		   addr, param->interval_min, (uint32_t)(param->interval_min * 1.25),
		   param->interval_max, (uint32_t)(param->interval_max * 1.25), param->latency,
		   param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_INFO("LE conn param updated: %s int 0x%04x (~%u ms) lat %d to %d", addr, interval,
		  (uint32_t)(interval * 1.25), latency, timeout);

	atomic_set_bit(conn_info.flags, CONN_INFO_CONN_PARAMS_UPDATED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

static void bt_ready(void)
{
	int err;

	TERM_PRINT("Bluetooth initialized");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		TERM_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	TERM_SUCCESS("Advertising successfully started");
}

void main(void)
{
	struct bt_gatt_attr *vnd_ind_attr;
	char str[BT_UUID_STR_LEN];
	int err;

	err = bt_enable(NULL);
	if (err) {
		TERM_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	bt_ready();

	bt_gatt_cb_register(&gatt_callbacks);

	vnd_ind_attr = bt_gatt_find_by_uuid(vnd_svc.attrs, vnd_svc.attr_count, &vnd_enc_uuid.uuid);

	bt_uuid_to_str(&vnd_enc_uuid.uuid, str, sizeof(str));
	TERM_PRINT("Indicate VND attr %p (UUID %s)", vnd_ind_attr, str);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (true) {

		if (conn_info.conn_ref == NULL) {
			k_sleep(K_MSEC(10));
			continue;
		}

		if (atomic_test_bit(conn_info.flags, CONN_INFO_CONN_PARAMS_UPDATED) == false) {
			k_sleep(K_MSEC(10));
			continue;
		}

		if (atomic_test_bit(conn_info.flags, CONN_INFO_MTU_EXCHANGED) == false) {
			k_sleep(K_MSEC(10));
			continue;
		}

		k_sleep(K_SECONDS(1));

		/* Vendor indication simulation */
		if (simulate_vnd && vnd_ind_attr) {
			memset(vnd_value, 0x00, sizeof(vnd_value));
			snprintk(vnd_value, NOTIFICATION_DATA_LEN, "%s%u", NOTIFICATION_DATA_PREFIX,
				 tx_notify_counter++);
			err = bt_gatt_notify(NULL, vnd_ind_attr, vnd_value, NOTIFICATION_DATA_LEN);
			if (err) {
				TERM_ERR("Couldn't send GATT notification");
			}
		}
	}
}
