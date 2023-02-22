/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/att.h>

CREATE_FLAG(flag_discover_complete);

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_is_connected);

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != g_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(g_conn);

	g_conn = NULL;
	UNSET_FLAG(flag_is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static uint16_t chrc_handle;

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		if (chrc_handle == 0) {
			FAIL("Did not discover chrc (%x)", chrc_handle);
		}

		(void)memset(params, 0, sizeof(*params));

		SET_FLAG(flag_discover_complete);

		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY &&
	    bt_uuid_cmp(params->uuid, TEST_SERVICE_UUID) == 0) {
		printk("Found test service\n");
		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, params);
		if (err != 0) {
			FAIL("Discover failed (err %d)\n", err);
		}

		return BT_GATT_ITER_STOP;
	} else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		const struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, TEST_CHRC_UUID) == 0) {
			printk("Found chrc value\n");
			chrc_handle = chrc->value_handle;
			params->type = BT_GATT_DISCOVER_DESCRIPTOR;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(void)
{
	struct bt_gatt_discover_params discover_params;
	int err;

	printk("Discovering services and characteristics\n");

	discover_params.uuid = TEST_SERVICE_UUID;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.chan_opt = BT_ATT_CHAN_OPT_NONE;

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		FAIL("Discover failed(err %d)\n", err);
	}

	printk("Discovery complete\n");
	WAIT_FOR_FLAG(flag_discover_complete);
}

static uint8_t notify_cb(struct bt_conn *conn,
			 struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	if (!data) {
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

void subscribed_cb(struct bt_conn *conn, uint8_t err,
		   struct bt_gatt_subscribe_params *params)
{
	printk("Subscribed ccc %x val %x\n",
	       params->value_handle,
	       params->ccc_handle);

	printk("Sending sync to peer\n");
	device_sync_send();
}

static struct bt_gatt_discover_params disc_params;
static struct bt_gatt_subscribe_params subscribe_params;
static void gatt_subscribe(void)
{
	int err;

	subscribe_params.value_handle = chrc_handle;
	subscribe_params.notify = notify_cb;
	subscribe_params.subscribe = subscribed_cb;

	/* Use the BT_GATT_AUTO_DISCOVER_CCC feature */
	subscribe_params.ccc_handle = 0;
	subscribe_params.disc_params = &disc_params,
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	subscribe_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	subscribe_params.chan_opt = BT_ATT_CHAN_OPT_NONE;

	printk("Subscribing: val %x\n", chrc_handle);
	err = bt_gatt_subscribe(g_conn, &subscribe_params);
	if (err != 0) {
		FAIL("Subscription failed(err %d)\n", err);
	}
}

static void test_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
	};

	device_sync_init(CENTRAL_ID);

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	/* Wait for the channels to be connected */
	while (bt_eatt_count(g_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_TICKS(1));
	}

	/* Subscribe to the server characteristic. */
	gatt_discover();
	gatt_subscribe();

	printk("Waiting for final sync\n");
	device_sync_wait();

	PASS("Server Passed\n");
}

static const struct bst_test_instance test_server[] = {
	{
		.test_id = "server",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_server);
}
