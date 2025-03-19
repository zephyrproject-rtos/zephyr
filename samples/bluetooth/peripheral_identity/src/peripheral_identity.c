/* main.c - Application main entry point */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

static struct k_work work_adv_start;
static uint8_t conn_count_max;
static uint8_t volatile conn_count;
static uint8_t id_current;
static bool volatile is_disconnecting;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void adv_start(struct k_work *work)
{
	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0,
		.secondary_max_skip = 0,
		.options = BT_LE_ADV_OPT_CONN,
		.interval_min = 0x0020, /* 20 ms */
		.interval_max = 0x0020, /* 20 ms */
		.peer = NULL,
	};
	size_t id_count = 0xFF;
	int err;

	bt_id_get(NULL, &id_count);
	if (id_current == id_count) {
		int id;

		id = bt_id_create(NULL, NULL);
		if (id < 0) {
			printk("Create id failed (%d)\n", id);
			if (id_current == 0) {
				id_current = conn_count_max;
			}
			id_current--;
		} else {
			printk("New id: %d\n", id);
		}
	}

	printk("Using current id: %u\n", id_current);
	adv_param.id = id_current;

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	id_current++;
	if (id_current == conn_count_max) {
		id_current = 0;
	}

	printk("Advertising successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	conn_count++;
	if (conn_count < conn_count_max) {
		k_work_submit(&work_adv_start);
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Connected (%u): %s\n", conn_count, addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected %s, reason %s(0x%02x)\n", addr, bt_hci_err_to_str(reason), reason);

	if ((conn_count == 1U) && is_disconnecting) {
		is_disconnecting = false;
		k_work_submit(&work_adv_start);
	}
	conn_count--;
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("LE conn  param req: %s int (0x%04x, 0x%04x) lat %d to %d\n",
	       addr, param->interval_min, param->interval_max, param->latency,
	       param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("LE conn param updated: %s int 0x%04x lat %d to %d\n",
	       addr, interval, latency, timeout);
}

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static void le_phy_updated(struct bt_conn *conn,
			   struct bt_conn_le_phy_info *param)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("LE PHY Updated: %s Tx 0x%x, Rx 0x%x\n", addr, param->tx_phy,
	       param->rx_phy);
}
#endif /* CONFIG_BT_USER_PHY_UPDATE */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static void le_data_len_updated(struct bt_conn *conn,
				struct bt_conn_le_data_len_info *info)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Data length updated: %s max tx %u (%u us) max rx %u (%u us)\n",
	       addr, info->tx_max_len, info->tx_max_time, info->rx_max_len,
	       info->rx_max_time);
}
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %s(%d)\n", addr, level,
		       bt_security_err_to_str(err), err);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_callbacks = {
	.cancel = auth_cancel,
};
#endif /* CONFIG_BT_SMP */

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,

#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	.le_phy_updated = le_phy_updated,
#endif /* CONFIG_BT_USER_PHY_UPDATE */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	.le_data_len_updated = le_data_len_updated,
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */
};

#if defined(CONFIG_BT_OBSERVER)
#define BT_LE_SCAN_PASSIVE_ALLOW_DUPILCATES \
		BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, \
				 BT_LE_SCAN_OPT_NONE, \
				 BT_GAP_SCAN_FAST_INTERVAL, \
				 BT_GAP_SCAN_FAST_INTERVAL)

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);
}
#endif /* CONFIG_BT_OBSERVER */

static void disconnect(struct bt_conn *conn, void *data)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnecting %s...\n", addr);
	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		printk("Failed disconnection %s.\n", addr);
		return;
	}
	printk("success.\n");
}

int init_peripheral(uint8_t max_conn, uint8_t iterations)
{
	size_t id_count;
	int err;

	conn_count_max = max_conn;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	bt_conn_cb_register(&conn_callbacks);

#if defined(CONFIG_BT_SMP)
	bt_conn_auth_cb_register(&auth_callbacks);
#endif /* CONFIG_BT_SMP */

	printk("Bluetooth initialized\n");

#if defined(CONFIG_BT_OBSERVER)
	printk("Start continuous passive scanning...");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_ALLOW_DUPILCATES,
			       device_found);
	if (err) {
		printk("Scan start failed (%d).\n", err);
		return err;
	}
	printk("success.\n");
#endif /* CONFIG_BT_OBSERVER */

	k_work_init(&work_adv_start, adv_start);
	k_work_submit(&work_adv_start);

	/* wait for connection attempts on all identities */
	do {
		k_sleep(K_MSEC(10));

		id_count = 0xFF;
		bt_id_get(NULL, &id_count);
	} while (id_count != conn_count_max);

	/* rotate identities so reconnections are attempted in case of any
	 * disconnections
	 */
	uint8_t prev_count = conn_count;
	while (1) {
		/* If maximum connections is reached, wait for disconnections
		 */
		if (conn_count == conn_count_max) {
			is_disconnecting = true;

			/* Lets wait sufficiently to ensure a stable connection
			 * before starting to disconnect for next iteration.
			 */
			k_sleep(K_SECONDS(60));

			if (!iterations) {
				break;
			}
			iterations--;
			printk("Iterations remaining: %u\n", iterations);

			/* Device needing multiple connections is the one
			 * initiating the disconnects.
			 */
			if (conn_count_max > 1U) {
				printk("Disconnecting all...\n");
				bt_conn_foreach(BT_CONN_TYPE_LE, disconnect, NULL);
			} else {
				printk("Wait for disconnections...\n");
			}

			while (is_disconnecting) {
				k_sleep(K_MSEC(10));
			}
			printk("All disconnected.\n");

			continue;
		}

		/* As long as there is connection count changes, identity
		 * rotation in this loop is not needed.
		 */
		if (prev_count != conn_count) {
			prev_count = conn_count;

			continue;
		} else {
			uint16_t wait = 6200U;

			/* Maximum duration without connection count change,
			 * central waiting before disconnecting all its
			 * connections plus few seconds of margin.
			 */
			while ((prev_count == conn_count) && wait) {
				printk("Waiting connections (%u/%u) %u...\n",
				       prev_count, conn_count, wait);

				wait--;

				k_sleep(K_MSEC(10));
			}

			if (wait) {
				continue;
			}
		}

		/* Stopping and restarting advertising to use new identity */
		printk("Stop advertising...\n");
		err = bt_le_adv_stop();
		if (err) {
			printk("Failed to stop advertising (%d)\n", err);

			return err;
		}

		k_work_submit(&work_adv_start);
	}

	return 0;
}
