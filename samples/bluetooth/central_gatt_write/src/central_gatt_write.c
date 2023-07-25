/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

extern int mtu_exchange(struct bt_conn *conn);
extern int write_cmd(struct bt_conn *conn);
extern struct bt_conn *conn_connected;
extern uint32_t last_write_rate;
extern void (*start_scan_func)(void);

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];
	struct bt_conn *conn;
	int err;

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	       dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	/* connect only to devices in close proximity */
	if (rssi < -70) {
		return;
	}

	err = bt_le_scan_stop();
	if (err) {
		printk("%s: Stop LE scan failed (err %d)\n", __func__, err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &conn);
	if (err) {
		printk("%s: Create conn failed (err %d)\n", __func__, err);
		start_scan_func();
	} else {
		bt_conn_unref(conn);
	}
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		printk("%s: Scanning failed to start (err %d)\n", __func__,
		       err);
		return;
	}

	printk("%s: Scanning successfully started\n", __func__);
}

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated
};

uint32_t central_gatt_write(uint32_t count)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0U;
	}
	printk("Bluetooth initialized\n");

	bt_gatt_cb_register(&gatt_callbacks);

	conn_connected = NULL;
	last_write_rate = 0U;

	start_scan_func = start_scan;
	start_scan_func();

	while (true) {
		struct bt_conn *conn = NULL;

		if (conn_connected) {
			/* Get a connection reference to ensure that a
			 * reference is maintained in case disconnected
			 * callback is called while we perform GATT Write
			 * command.
			 */
			conn = bt_conn_ref(conn_connected);
		}

		if (conn) {
			(void)write_cmd(conn);
			bt_conn_unref(conn);

			if (count) {
				count--;
				if (!count) {
					break;
				}
			}

			k_yield();
		} else {
			k_sleep(K_SECONDS(1));
		}
	}

	return last_write_rate;
}
