/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/util.h>

#define NUM_RSP_SLOTS	  5
#define NUM_SUBEVENTS	  5
#define PACKET_SIZE	  5
#define SUBEVENT_INTERVAL 0x30

static K_SEM_DEFINE(sem_scan_found, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_disconnected, 0, 1);

static bt_addr_le_t peer_addr;

static const struct bt_le_per_adv_param per_adv_params = {
	.interval_min = 0xFF,
	.interval_max = 0xFF,
	.options = 0,
	.num_subevents = NUM_SUBEVENTS,
	.subevent_interval = SUBEVENT_INTERVAL,
	.response_slot_delay = 0x8,
	.response_slot_spacing = 0x50,
	.num_response_slots = NUM_RSP_SLOTS,
};

static struct bt_le_per_adv_subevent_data_params subevent_data_params[NUM_SUBEVENTS];
static struct net_buf_simple bufs[NUM_SUBEVENTS];
static uint8_t backing_store[NUM_SUBEVENTS][PACKET_SIZE];

BUILD_ASSERT(ARRAY_SIZE(bufs) == ARRAY_SIZE(subevent_data_params));
BUILD_ASSERT(ARRAY_SIZE(backing_store) == ARRAY_SIZE(subevent_data_params));

static void request_cb(struct bt_le_ext_adv *adv, const struct bt_le_per_adv_data_request *request)
{
	int err;
	uint8_t to_send;

	/* Continuously send the same dummy data and listen to all response slots */

	to_send = MIN(request->count, ARRAY_SIZE(subevent_data_params));
	for (size_t i = 0; i < to_send; i++) {
		subevent_data_params[i].subevent =
			(request->start + i) % per_adv_params.num_subevents;
		subevent_data_params[i].response_slot_start = 0;
		subevent_data_params[i].response_slot_count = NUM_RSP_SLOTS;
		subevent_data_params[i].data = &bufs[i];
	}

	err = bt_le_per_adv_set_subevent_data(adv, to_send, subevent_data_params);
	if (err != 0) {
		printk("Failed to set subevent data (err %d)\n", err);
	}
}

static struct bt_conn *default_conn;

static bool response_data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	if (data->type == BT_DATA_NAME_COMPLETE &&
	    data->data_len == sizeof(CONFIG_SAMPLE_PERIODIC_ADV_CONN_PEER_NAME) - 1) {
		len = data->data_len;
		(void)memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	}

	return true;
}

static void response_cb(struct bt_le_ext_adv *adv, struct bt_le_per_adv_response_info *info,
			struct net_buf_simple *buf)
{
	int err;
	char name[sizeof(CONFIG_SAMPLE_PERIODIC_ADV_CONN_PEER_NAME)];
	struct bt_conn_le_create_synced_param synced_param;
	struct bt_le_conn_param conn_param;

	if (buf == NULL || buf->len == 0U) {
		return;
	}

	if (default_conn != NULL) {
		/* Do not initiate new connections while already connected */
		return;
	}

	(void)memset(name, 0, sizeof(name));
	bt_data_parse(buf, response_data_cb, name);
	if (strcmp(name, CONFIG_SAMPLE_PERIODIC_ADV_CONN_PEER_NAME) != 0) {
		return;
	}

	/* Address was already learned via the initial connection (see main()) */
	printk("Connecting to %s in subevent %d\n", bt_addr_le_str(&peer_addr), info->subevent);

	synced_param.peer = &peer_addr;
	synced_param.subevent = info->subevent;

	/* Choose same interval as PAwR advertiser to avoid scheduling conflicts */
	conn_param.interval_min = SUBEVENT_INTERVAL;
	conn_param.interval_max = SUBEVENT_INTERVAL;

	/* Default values */
	conn_param.latency = 0;
	conn_param.timeout = 400;

	err = bt_conn_le_create_synced(adv, &synced_param, &conn_param, &default_conn);
	if (err != 0) {
		printk("Failed to initiate connection (err %d)\n", err);
	}
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.pawr_data_request = request_cb,
	.pawr_response = response_cb,
};

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	printk("Connected (err 0x%02X)\n", err);

	__ASSERT(conn == default_conn, "Unexpected connected callback");

	if (err != 0) {
		bt_conn_drop(&default_conn);
	}

	k_sem_give(&sem_connected);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02X %s\n", reason, bt_hci_err_to_str(reason));

	__ASSERT(conn == default_conn, "Unexpected disconnected callback");

	bt_conn_drop(&default_conn);

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
		len = MIN(data->data_len,
			  sizeof(CONFIG_SAMPLE_PERIODIC_ADV_CONN_PEER_NAME) - 1);
		(void)memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	char name[sizeof(CONFIG_SAMPLE_PERIODIC_ADV_CONN_PEER_NAME)];

	(void)memset(name, 0, sizeof(name));
	bt_data_parse(buf, data_cb, name);

	if (strcmp(name, CONFIG_SAMPLE_PERIODIC_ADV_CONN_PEER_NAME) != 0) {
		return;
	}

	bt_addr_le_copy(&peer_addr, info->addr);

	k_sem_give(&sem_scan_found);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void init_bufs(void)
{
	/* Set up some dummy data to send */
	for (size_t i = 0; i < ARRAY_SIZE(backing_store); i++) {
		backing_store[i][0] = ARRAY_SIZE(backing_store[i]) - 1;
		backing_store[i][1] = BT_DATA_MANUFACTURER_DATA;
		backing_store[i][2] = 0x59; /* Nordic */
		backing_store[i][3] = 0x00;

		net_buf_simple_init_with_data(&bufs[i], &backing_store[i],
					      ARRAY_SIZE(backing_store[i]));
	}
}

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int main(void)
{
	int err;
	struct bt_le_ext_adv *pawr_adv;

	init_bufs();

	printk("Starting Periodic Advertising Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_le_scan_cb_register(&scan_callbacks);

	/* Connect once to learn the periodic_sync_conn sample's address ahead of time */
	printk("Scanning for periodic_sync_conn sample\n");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		printk("Scanning failed to start (err %d)\n", err);
		return 0;
	}

	err = k_sem_take(&sem_scan_found, K_FOREVER);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return 0;
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		printk("Failed to stop scanning (err %d)\n", err);
		return 0;
	}

	printk("Connecting to %s to learn its address\n", bt_addr_le_str(&peer_addr));
	err = bt_conn_le_create(&peer_addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err != 0) {
		printk("Connection failed (err %d)\n", err);
		return 0;
	}

	err = k_sem_take(&sem_connected, K_FOREVER);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return 0;
	}

	if (default_conn == NULL) {
		printk("Connection failed\n");
		return 0;
	}

	printk("Disconnecting\n");
	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		printk("Disconnect failed (err %d)\n", err);
		return 0;
	}

	err = k_sem_take(&sem_disconnected, K_FOREVER);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return 0;
	}

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &adv_cb, &pawr_adv);
	if (err != 0) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(pawr_adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(pawr_adv, &per_adv_params);
	if (err != 0) {
		printk("Failed to set periodic advertising parameters (err %d)\n", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(pawr_adv);
	if (err != 0) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return 0;
	}

	printk("Start Periodic Advertising\n");
	err = bt_le_ext_adv_start(pawr_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return 0;
	}

	while (true) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
