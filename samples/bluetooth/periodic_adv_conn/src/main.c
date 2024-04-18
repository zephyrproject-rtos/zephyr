/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>

#define NUM_RSP_SLOTS	  5
#define NUM_SUBEVENTS	  5
#define PACKET_SIZE	  5
#define SUBEVENT_INTERVAL 0x30

static const struct bt_le_per_adv_param per_adv_params = {
	.interval_min = 0xFF,
	.interval_max = 0xFF,
	.options = 0,
	.num_subevents = NUM_SUBEVENTS,
	.subevent_interval = SUBEVENT_INTERVAL,
	.response_slot_delay = 0x5,
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
	if (err) {
		printk("Failed to set subevent data (err %d)\n", err);
	}
}

static bool get_address(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;

	if (data->type == BT_DATA_LE_BT_DEVICE_ADDRESS) {
		memcpy(addr->a.val, data->data, sizeof(addr->a.val));
		addr->type = data->data[sizeof(addr->a)];

		return false;
	}

	return true;
}

static struct bt_conn *default_conn;

static void response_cb(struct bt_le_ext_adv *adv, struct bt_le_per_adv_response_info *info,
			struct net_buf_simple *buf)
{
	int err;
	bt_addr_le_t peer;
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_conn_le_create_synced_param synced_param;
	struct bt_le_conn_param conn_param;

	if (!buf) {
		return;
	}

	if (default_conn) {
		/* Do not initiate new connections while already connected */
		return;
	}

	bt_addr_le_copy(&peer, &bt_addr_le_none);
	bt_data_parse(buf, get_address, &peer);
	if (bt_addr_le_eq(&peer, &bt_addr_le_none)) {
		/* No address found */
		return;
	}

	bt_addr_le_to_str(&peer, addr_str, sizeof(addr_str));
	printk("Connecting to %s in subevent %d\n", addr_str, info->subevent);

	synced_param.peer = &peer;
	synced_param.subevent = info->subevent;

	/* Choose same interval as PAwR advertiser to avoid scheduling conflicts */
	conn_param.interval_min = SUBEVENT_INTERVAL;
	conn_param.interval_max = SUBEVENT_INTERVAL;

	/* Default values */
	conn_param.latency = 0;
	conn_param.timeout = 400;

	err = bt_conn_le_create_synced(adv, &synced_param, &conn_param, &default_conn);
	if (err) {
		printk("Failed to initiate connection (err %d)", err);
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

	if (err) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02X)\n", reason);

	__ASSERT(conn == default_conn, "Unexpected disconnected callback");

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
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
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &adv_cb, &pawr_adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(pawr_adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(pawr_adv, &per_adv_params);
	if (err) {
		printk("Failed to set periodic advertising parameters (err %d)\n", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(pawr_adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return 0;
	}

	printk("Start Periodic Advertising\n");
	err = bt_le_ext_adv_start(pawr_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return 0;
	}

	while (true) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
