/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

/* Interval between printing throughput metrics */
#define METRICS_INTERVAL 1U /* seconds */

/* Custom service UUID: 12345678-1234-5678-1234-56789abcdef0 */
#define BT_UUID_NOTIFY_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_NOTIFY_SERVICE BT_UUID_DECLARE_128(BT_UUID_NOTIFY_SERVICE_VAL)

/* Custom characteristic UUID: 12345678-1234-5678-1234-56789abcdef1 */
#define BT_UUID_NOTIFY_CHAR_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)
#define BT_UUID_NOTIFY_CHAR BT_UUID_DECLARE_128(BT_UUID_NOTIFY_CHAR_VAL)

static struct bt_conn *periph_conn_connected;
static bool periph_notify_enabled;
static bool periph_role_active;

/* Throughput metrics */
static uint32_t periph_notify_count;
static uint32_t periph_notify_len;
static uint32_t periph_notify_rate;
uint32_t periph_last_notify_rate;

static void periph_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	periph_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	printk("[Peripheral] Notifications %s\n", periph_notify_enabled ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(periph_notify_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_NOTIFY_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_NOTIFY_CHAR,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC(periph_ccc_cfg_changed,
		     BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static const struct bt_data periph_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void periph_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("[Peripheral] Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb periph_gatt_callbacks = {
	.att_mtu_updated = periph_mtu_updated,
};

static void periph_connected(struct bt_conn *conn, uint8_t conn_err)
{
	if (!periph_role_active) {
		return;
	}

	if (conn_err) {
		printk("[Peripheral] Failed to connect (0x%02x)\n", conn_err);
		return;
	}

	printk("[Peripheral] Connected: %s\n", bt_conn_dst_str(conn));
	periph_conn_connected = bt_conn_ref(conn);
}

static void periph_disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != periph_conn_connected) {
		return;
	}

	printk("[Peripheral] Disconnected: %s (reason 0x%02x)\n", bt_conn_dst_str(conn), reason);

	if (periph_conn_connected) {
		bt_conn_unref(periph_conn_connected);
		periph_conn_connected = NULL;
	}

	periph_notify_enabled = false;

	/* Restart advertising */
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, periph_ad, ARRAY_SIZE(periph_ad),
				  NULL, 0);

	if (err) {
		printk("[Peripheral] Advertising failed to restart (err %d)\n", err);
	}
}

static void periph_le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("[Peripheral] Conn params updated: int 0x%04x lat %u to %u\n", interval,
	       latency, timeout);
}

BT_CONN_CB_DEFINE(periph_notify_conn_cb) = {
	.connected = periph_connected,
	.disconnected = periph_disconnected,
	.le_param_updated = periph_le_param_updated,
};

static int periph_send_notification(struct bt_conn *conn)
{
	static uint8_t data[BT_ATT_MAX_ATTRIBUTE_LEN];
	uint16_t data_len;
	int err;

	data_len = bt_gatt_get_mtu(conn) - 3;
	if (data_len > BT_ATT_MAX_ATTRIBUTE_LEN) {
		data_len = BT_ATT_MAX_ATTRIBUTE_LEN;
	}

	/* Use the second attribute (the characteristic value) in the service */
	err = bt_gatt_notify(conn, &periph_notify_svc.attrs[1], data, data_len);
	if (err == -ENOMEM) {
		/* No buffers available, not an error - just back off */
		return err;
	}

	if (err) {
		printk("[Peripheral] Notify failed (err %d)\n", err);
		return err;
	}

	return data_len;
}

static void periph_update_metrics(uint16_t len)
{
	static uint32_t cycle_stamp;
	uint64_t delta;

	delta = k_cycle_get_32() - cycle_stamp;
	delta = k_cyc_to_ns_floor64(delta);

	if (delta == 0) {
		return;
	}

	if (delta > (METRICS_INTERVAL * NSEC_PER_SEC)) {
		printk("[Peripheral] Notify: count= %u, len= %u, rate= %u bps.\n",
		       periph_notify_count, periph_notify_len, periph_notify_rate);

		periph_last_notify_rate = periph_notify_rate;

		periph_notify_count = 0U;
		periph_notify_len = 0U;
		periph_notify_rate = 0U;
		cycle_stamp = k_cycle_get_32();
	} else {
		periph_notify_count++;
		periph_notify_len += len;
		periph_notify_rate = ((uint64_t)periph_notify_len << 3) *
			      (METRICS_INTERVAL * NSEC_PER_SEC) / delta;
	}
}

uint32_t peripheral_gatt_notify(uint32_t count)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("[Peripheral] Bluetooth init failed (err %d)\n", err);
		return 0U;
	}
	printk("[Peripheral] Bluetooth initialized\n");

	bt_gatt_cb_register(&periph_gatt_callbacks);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, periph_ad, ARRAY_SIZE(periph_ad), NULL, 0);
	if (err) {
		printk("[Peripheral] Advertising failed to start (err %d)\n", err);
		return 0U;
	}
	printk("[Peripheral] Advertising successfully started\n");

	periph_role_active = true;
	periph_conn_connected = NULL;
	periph_notify_enabled = false;
	periph_last_notify_rate = 0U;

	if (count != 0U) {
		printk("[Peripheral] GATT Notify countdown %u on connection.\n", count);
	} else {
		printk("[Peripheral] GATT Notify forever on connection.\n");
	}

	while (true) {
		struct bt_conn *conn = NULL;

		if (periph_conn_connected && periph_notify_enabled) {
			conn = bt_conn_ref(periph_conn_connected);
		}

		if (conn) {
			int ret = periph_send_notification(conn);

			bt_conn_unref(conn);

			if (ret > 0) {
				periph_update_metrics((uint16_t)ret);

				if (count) {
					count--;
					if (!count) {
						break;
					}
				}
			} else if (ret == -ENOMEM) {
				/* Wait briefly for buffers to free up */
				k_yield();
			}

			k_yield();
		} else {
			k_sleep(K_SECONDS(1));
		}
	}

	return periph_last_notify_rate;
}
