/**
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#define NAME_LEN 30

static struct bt_conn *default_conn;
bt_addr_le_t ext_addr;

enum bt_sample_scan_evt {
	BT_SAMPLE_EVT_EXT_ADV_FOUND,
	BT_SAMPLE_EVT_CONNECTED,
	BT_SAMPLE_EVT_DISCONNECTED,
	BT_SAMPLE_EVT_SCAN_DUE,
	BT_SAMPLE_EVT_MAX,
};

enum bt_sample_scan_st {
	BT_SAMPLE_ST_SCANNING,
	BT_SAMPLE_ST_CONNECTING,
	BT_SAMPLE_ST_CONNECTED,
	BT_SAMPLE_ST_COOLDOWN,
};

static volatile enum bt_sample_scan_st app_st = BT_SAMPLE_ST_SCANNING;

static ATOMIC_DEFINE(evt_bitmask, BT_SAMPLE_EVT_MAX);

static struct k_poll_signal poll_sig = K_POLL_SIGNAL_INITIALIZER(poll_sig);
static struct k_poll_event poll_evt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
						K_POLL_MODE_NOTIFY_ONLY, &poll_sig);

static void raise_evt(enum bt_sample_scan_evt evt)
{
	(void)atomic_set_bit(evt_bitmask, evt);
	k_poll_signal_raise(poll_evt.signal, 1);
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	printk("Connected (err 0x%02X)\n", err);

	if (err) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
		return;
	}

	raise_evt(BT_SAMPLE_EVT_CONNECTED);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_unref(default_conn);
	default_conn = NULL;

	printk("Disconnected, reason 0x%02X %s\n", reason, bt_hci_err_to_str(reason));
}

static void recycled_cb(void)
{
	printk("Connection object available from previous conn. Disconnect is complete!\n");
	raise_evt(BT_SAMPLE_EVT_DISCONNECTED);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.recycled = recycled_cb,
};


static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	if (info->adv_type & BT_GAP_ADV_TYPE_EXT_ADV &&
	    info->adv_props & BT_GAP_ADV_PROP_EXT_ADV &&
	    info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) {
		/* Attempt connection request for device with extended advertisements */
		memcpy(&ext_addr, info->addr, sizeof(ext_addr));
		raise_evt(BT_SAMPLE_EVT_EXT_ADV_FOUND);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static inline int attempt_connection(void)
{
	int err;

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err) {
		printk("Failed to stop scan: %d\n", err);
		return err;
	}

	err = bt_conn_le_create(&ext_addr, BT_CONN_LE_CREATE_CONN,
		BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err) {
		printk("Failed to establish conn: %d\n", err);
		return err;
	}

	return 0;
}

static inline int start_scanning(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err) {
		printk("failed (err %d)\n", err);
	}

	return err;
}

int main(void)
{
	int err;

	printk("Starting Extended Advertising Demo [Scanner]\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_le_scan_cb_register(&scan_callbacks);

	err = start_scanning();
	if (err) {
		return err;
	}

	while (true) {
		(void)k_poll(&poll_evt, 1, K_FOREVER);

		k_poll_signal_reset(poll_evt.signal);
		poll_evt.state = K_POLL_STATE_NOT_READY;

		/* Identify event and act upon if applicable */
		if (atomic_test_and_clear_bit(evt_bitmask, BT_SAMPLE_EVT_EXT_ADV_FOUND) &&
		    app_st == BT_SAMPLE_ST_SCANNING) {

			printk("Found extended advertisement packet!\n");
			app_st = BT_SAMPLE_ST_CONNECTING;
			err = attempt_connection();
			if (err) {
				return err;
			}

		} else if (atomic_test_and_clear_bit(evt_bitmask, BT_SAMPLE_EVT_CONNECTED) &&
			   (app_st == BT_SAMPLE_ST_CONNECTING)) {

			printk("Connected state!\n");
			app_st = BT_SAMPLE_ST_CONNECTED;

		} else if (atomic_test_and_clear_bit(evt_bitmask, BT_SAMPLE_EVT_DISCONNECTED) &&
			   (app_st == BT_SAMPLE_ST_CONNECTED)) {

			printk("Disconnected, cooldown for 5 seconds!\n");
			app_st = BT_SAMPLE_ST_COOLDOWN;

			/* Wait a few seconds before starting to re-scan again... */
			k_sleep(K_SECONDS(5));

			raise_evt(BT_SAMPLE_EVT_SCAN_DUE);

		} else if (atomic_test_and_clear_bit(evt_bitmask, BT_SAMPLE_EVT_SCAN_DUE) &&
			   (app_st == BT_SAMPLE_ST_COOLDOWN)) {

			printk("Starting to scan for extended adv\n");
			app_st = BT_SAMPLE_ST_SCANNING;
			err = start_scanning();
			if (err) {
				return err;
			}

		}
	}

	return 0;
}
