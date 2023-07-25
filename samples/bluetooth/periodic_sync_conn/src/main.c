/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/sys/util.h>

#define NAME_LEN 30

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_disconnected, 0, 1);

static struct bt_conn *default_conn;
static bool per_adv_found;
static bt_addr_le_t per_addr;
static uint8_t per_sid;

static void sync_cb(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	struct bt_le_per_adv_sync_subevent_params params;
	uint8_t subevents[1];
	char le_addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("Synced to %s with %d subevents\n", le_addr, info->num_subevents);

	params.properties = 0;
	params.num_subevents = 1;
	params.subevents = subevents;
	subevents[0] = 0;
	err = bt_le_per_adv_sync_subevent(sync, &params);
	if (err) {
		printk("Failed to set subevents to sync to (err %d)\n", err);
	}

	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("Sync terminated (reason %d)\n", info->reason);
}

static struct bt_le_per_adv_response_params rsp_params;

NET_BUF_SIMPLE_DEFINE_STATIC(rsp_buf, sizeof(bt_addr_le_t) + 2 * sizeof(uint8_t));

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
	int err;
	struct bt_le_oob oob;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn) {
		/* Only respond with address if not already connected */
		return;
	}

	if (buf && buf->len) {
		/* Respond with own address for the advertiser to connect to */
		net_buf_simple_reset(&rsp_buf);

		rsp_params.request_event = info->periodic_event_counter;
		rsp_params.request_subevent = info->subevent;
		rsp_params.response_subevent = info->subevent;
		rsp_params.response_slot = 0;

		err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
		if (err) {
			printk("Failed to get OOB data (err %d)\n", err);

			return;
		}

		bt_addr_le_to_str(&oob.addr, addr_str, sizeof(addr_str));
		printk("Responding with own addr: %s\n", addr_str);

		net_buf_simple_add_u8(&rsp_buf, sizeof(bt_addr_le_t));
		net_buf_simple_add_u8(&rsp_buf, BT_DATA_LE_BT_DEVICE_ADDRESS);
		net_buf_simple_add_mem(&rsp_buf, &oob.addr.a, sizeof(oob.addr.a));
		net_buf_simple_add_u8(&rsp_buf, oob.addr.type);

		err = bt_le_per_adv_set_response_data(sync, &rsp_params, &rsp_buf);
		if (err) {
			printk("Failed to send response (err %d)\n", err);
		}
	} else if (buf) {
		printk("Received empty indication: subevent %d\n", info->subevent);
	} else {
		printk("Failed to receive indication: subevent %d\n", info->subevent);
	}
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb,
};

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	printk("Connected (err 0x%02X)\n", err);

	if (err) {
		return;
	}

	default_conn = bt_conn_ref(conn);
	k_sem_give(&sem_connected);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_unref(default_conn);
	default_conn = NULL;

	printk("Disconnected (reason 0x%02X)\n", reason);

	k_sem_give(&sem_disconnected);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

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

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));
	bt_data_parse(buf, data_cb, name);

	if (strcmp(name, "PAwR conn sample")) {
		return;
	}

	if (!per_adv_found && info->interval) {
		per_adv_found = true;

		per_sid = info->sid;
		bt_addr_le_copy(&per_addr, info->addr);

		k_sem_give(&sem_per_adv);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

int main(void)
{
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync;
	int err;

	printk("Starting Periodic Advertising with Responses Synchronization Demo\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);

		return 0;
	}

	bt_le_scan_cb_register(&scan_callbacks);
	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err) {
		printk("failed (err %d)\n", err);

		return 0;
	}

	err = k_sem_take(&sem_per_adv, K_FOREVER);
	if (err) {
		printk("failed (err %d)\n", err);

		return 0;
	}
	printk("Found periodic advertising.\n");

	printk("Creating Periodic Advertising Sync");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xaa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		printk("Failed to create sync (err %d)\n", err);

		return 0;
	}

	printk("Waiting for periodic sync\n");
	err = k_sem_take(&sem_per_sync, K_FOREVER);
	if (err) {
		printk("Failed (err %d)\n", err);

		return 0;
	}

	printk("Periodic sync established.\n");

	err = bt_le_scan_stop();
	if (err) {
		printk("Failed to stop scanning (err %d)\n", err);
	}

	printk("Stopped scanning\n");

	do {
		err = k_sem_take(&sem_connected, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);

			return 0;
		}

		printk("Disconnecting\n");

		err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			return 0;
		}

		err = k_sem_take(&sem_disconnected, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);

			return 0;
		}
	} while (true);
}
