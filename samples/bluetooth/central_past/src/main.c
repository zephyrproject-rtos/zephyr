/*
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/util.h>

#define NAME_LEN            30

static bool per_adv_found;
static bt_addr_le_t per_addr;
static uint8_t per_sid;
static struct bt_conn *default_conn;
static uint16_t per_adv_sync_timeout;

static K_SEM_DEFINE(sem_conn, 0, 1);
static K_SEM_DEFINE(sem_conn_lost, 0, 1);
static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];
	int err;

	/* only parse devices in close proximity */
	if (info->rssi < -70) {
		return;
	}

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i, name: %s "
	       "C:%u S:%u D:%u SR:%u E:%u Prim: %s, Secn: %s, "
	       "Interval: 0x%04x (%u ms), SID: %u\n",
	       le_addr, info->adv_type, info->tx_power, info->rssi, name,
	       (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
	       phy2str(info->primary_phy), phy2str(info->secondary_phy),
	       info->interval, info->interval * 5 / 4, info->sid);

	/* If connectable, connect */
	if (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) {
		if (default_conn) {
			return;
		}

		printk("Connecting to %s\n", le_addr);

		err = bt_le_scan_stop();
		if (err != 0) {
			printk("Stop LE scan failed (err %d)\n", err);
			return;
		}

		err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT,
					&default_conn);
		if (err != 0) {
			printk("Failed to connect (err %d)\n", err);
			return;
		}
	} else {
		/* If info->interval it is a periodic advertiser, mark for sync */
		if (!per_adv_found && info->interval) {
			uint32_t interval_us;
			uint32_t timeout;

			per_adv_found = true;

			/* Add retries and convert to unit in 10's of ms */
			interval_us = BT_GAP_PER_ADV_INTERVAL_TO_US(info->interval);

			timeout = BT_GAP_US_TO_PER_ADV_SYNC_TIMEOUT(interval_us);

			/* 10 attempts */
			timeout *= 10;

			/* Enforce restraints */
			per_adv_sync_timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT,
						     BT_GAP_PER_ADV_MAX_TIMEOUT);

			per_sid = info->sid;
			bt_addr_le_copy(&per_addr, info->addr);

			k_sem_give(&sem_per_adv);
		}
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int bt_err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));

		bt_conn_unref(default_conn);
		default_conn = NULL;


		bt_err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
		if (bt_err) {
			printk("Failed to start scan (err %d)\n", bt_err);
			return;
		}

		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);

	k_sem_give(&sem_conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	bt_conn_unref(default_conn);
	default_conn = NULL;

	k_sem_give(&sem_conn_lost);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		printk("Failed to start scan (err %d)\n", err);
		return;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void sync_cb(struct bt_le_per_adv_sync *sync,
		    struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u ms), PHY %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr,
	       info->interval, info->interval * 5 / 4, phy2str(info->phy));

	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char data_str[129];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	bin2hex(buf->data, buf->len, data_str, sizeof(data_str));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
	       "RSSI %i, CTE %u, data length %u, data: %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
	       info->rssi, info->cte_type, buf->len, data_str);
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb
};

int main(void)
{
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync;
	int err;
	char le_addr[BT_ADDR_LE_STR_LEN];

	printk("Starting Central Periodic Advertising Synchronization Transfer (PAST) Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err != 0) {
		printk("failed to enable BT (err %d)\n", err);
		return 0;
	}

	printk("Connection callbacks register\n");
	bt_conn_cb_register(&conn_callbacks);

	printk("Scan callbacks register\n");
	bt_le_scan_cb_register(&scan_callbacks);

	printk("Periodic Advertising callbacks register\n");
	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	printk("Start scanning...");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return 0;
	}
	printk("success.\n");

	do {
		printk("Waiting for connection...\n");
		err = k_sem_take(&sem_conn, K_FOREVER);
		if (err != 0) {
			printk("Could not take sem_conn (err %d)\n", err);
			return 0;
		}
		printk("Connected.\n");

		printk("Start scanning for PA...\n");
		per_adv_found = false;
		err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
		if (err != 0) {
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("Scan started.\n");

		printk("Waiting for periodic advertising...\n");
		err = k_sem_take(&sem_per_adv, K_FOREVER);
		if (err != 0) {
			printk("Could not take sem_per_adv (err %d)\n", err);
			return 0;
		}
		printk("Found periodic advertising.\n");

		bt_addr_le_to_str(&per_addr, le_addr, sizeof(le_addr));
		printk("Creating Periodic Advertising Sync to %s...\n", le_addr);
		bt_addr_le_copy(&sync_create_param.addr, &per_addr);
		sync_create_param.options = 0;
		sync_create_param.sid = per_sid;
		sync_create_param.skip = 0;
		sync_create_param.timeout = per_adv_sync_timeout;
		err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
		if (err != 0) {
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("success.\n");

		printk("Waiting for periodic sync...\n");
		err = k_sem_take(&sem_per_sync, K_FOREVER);
		if (err != 0) {
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("Periodic sync established.\n");

		printk("Transferring sync\n");
		err = bt_le_per_adv_sync_transfer(sync, default_conn, 0);
		if (err != 0) {
			printk("Could not transfer sync (err %d)\n", err);
			return 0;
		}

		printk("Waiting for connection lost...\n");
		err = k_sem_take(&sem_conn_lost, K_FOREVER);
		if (err != 0) {
			printk("Could not take sem_conn_lost (err %d)\n", err);
			return 0;
		}
		printk("Connection lost.\n");
	} while (true);
}
