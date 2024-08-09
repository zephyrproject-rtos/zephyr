/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2017-2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"
#include <argparse.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

static struct bt_conn *default_conn;

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;

static struct bt_gatt_subscribe_params battery_level_notify_params;
static struct bt_gatt_subscribe_params battery_level_status_sub_params;

/* Define custom return values for the callback */
#define BT_GATT_SUBSCRIBE_OK  0
#define BT_GATT_SUBSCRIBE_ERR 1

/*
 * Battery Service  test:
 *   We expect to find a connectable peripheral to which we will
 *   connect and discover Battery Service
 *
 *   Test the Read/Notify/Indicate Characteristics of BAS
 */

#define WAIT_TIME                  15 /*seconds*/
#define BAS_BLS_IND_RECEIVED_COUNT 10
#define BAS_BLS_NTF_RECEIVED_COUNT 10

extern enum bst_result_t bst_result;

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS() (bst_result = Passed)

static void test_bas_central_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME * 1e6);
	bst_result = In_progress;
}

static void test_bas_central_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		FAIL("test_bas_central failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

/* Callback for handling Battery Level Notifications */
static uint8_t battery_level_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t length)
{
	if (data) {
		printk("[NOTIFICATION] BAS Battery Level: %d%%\n", *(const uint8_t *)data);
	} else {
		printk("Battery Level Notifications disabled\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

/* Callback for handling Battery Level Read Response */
static uint8_t battery_level_read_cb(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_read_params *params, const void *data,
				     uint16_t length)
{
	if (err) {
		printk("Failed to read Battery Level (err %u)\n", err);
		return BT_GATT_ITER_STOP;
	}

	if (data) {
		printk("[READ] BAS Battery Level: %d%%\n", *(const uint8_t *)data);
	}

	return BT_GATT_ITER_STOP;
}

static unsigned char battery_level_status_indicate_cb(struct bt_conn *conn,
						      struct bt_gatt_subscribe_params *params,
						      const void *data, uint16_t length)
{
	static int ind_received;

	if (!data) {
		printk("bas level status indication disabled\n");
	} else {
		printk("[INDICATION] BAS Battery Level Status: ");
		for (int i = 0; i < length; i++) {
			printk("%02x ", ((uint8_t *)data)[i]);
		}
		printk("\n");

		if (ind_received++ > BAS_BLS_IND_RECEIVED_COUNT) {
			PASS();
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t battery_level_status_notify_cb(struct bt_conn *conn,
					      struct bt_gatt_subscribe_params *params,
					      const void *data, uint16_t length)
{
	static int notify_count;

	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	printk("[NOTIFICATION] BAS Battery Level Status: ");
	for (int i = 0; i < length; i++) {
		printk("%02x ", ((uint8_t *)data)[i]);
	}
	printk("\n");

	if (notify_count++ >= BAS_BLS_NTF_RECEIVED_COUNT) { /* We consider it passed */
		PASS();
	}
	return BT_GATT_ITER_CONTINUE;
}

static void read_battery_level(const struct bt_gatt_attr *attr)
{

	/* Read the battery level after subscribing */
	static struct bt_gatt_read_params read_params;

	read_params.func = battery_level_read_cb;
	read_params.handle_count = 1;
	read_params.single.handle = bt_gatt_attr_get_handle(attr);
	read_params.single.offset = 0;
	bt_gatt_read(default_conn, &read_params);
}

static void subscribe_battery_level(const struct bt_gatt_attr *attr)
{
	int err;

	battery_level_notify_params = (struct bt_gatt_subscribe_params){
		.ccc_handle = bt_gatt_attr_get_handle(attr) + 2,
		.value_handle = bt_gatt_attr_get_handle(attr) + 1,
		.value = BT_GATT_CCC_NOTIFY,
		.notify = battery_level_notify_cb,
	};

	err = bt_gatt_subscribe(default_conn, &battery_level_notify_params);
	if (err && err != -EALREADY) {
		FAIL("Subscribe failed (err %d)\n", err);
	} else {
		printk("Battery level [SUBSCRIBED]\n");
	}
	read_battery_level(attr);
}

static void subscribe_battery_level_status(const struct bt_gatt_attr *attr)
{
	int err;

	if (get_device_nbr() < 3) { /* First three devices: Indications */
		battery_level_status_sub_params = (struct bt_gatt_subscribe_params){
			.ccc_handle = bt_gatt_attr_get_handle(attr),
			.value_handle = bt_gatt_attr_get_handle(attr) - 1,
			.value = BT_GATT_CCC_INDICATE,
			.notify = battery_level_status_indicate_cb,
		};
	} else { /* Last two devices: Notifications */
		battery_level_status_sub_params = (struct bt_gatt_subscribe_params){
			.ccc_handle = bt_gatt_attr_get_handle(attr),
			.value_handle = bt_gatt_attr_get_handle(attr) - 1,
			.value = BT_GATT_CCC_NOTIFY,
			.notify = battery_level_status_notify_cb,
		};
	}

	err = bt_gatt_subscribe(default_conn, &battery_level_status_sub_params);
	if (err && err != -EALREADY) {
		FAIL("Subscribe failed (err %d)\n", err);
	} else {
		printk("Battery level status [SUBSCRIBED]\n");
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_BAS)) {
		printk("battery service\n");
		memcpy(&uuid, BT_UUID_BAS_BATTERY_LEVEL, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			FAIL("Discover failed (err %d)\n", err);
		}

	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_BAS_BATTERY_LEVEL)) {
		printk("battery level char\n");
		printk("subscribe battery level\n");
		subscribe_battery_level(attr);

		memcpy(&uuid, BT_UUID_BAS_BATTERY_LEVEL_STATUS, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			FAIL("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_BAS_BATTERY_LEVEL_STATUS)) {
		printk("batterry level status char\n");
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			FAIL("Discover failed (err %d)\n", err);
		}
	} else {
		printk("batterry level status char ccc handle\n");
		subscribe_battery_level_status(attr);
	}

	return BT_GATT_ITER_STOP;
}

static void discover_bas_service(struct bt_conn *conn)
{
	int err;

	printk("%s\n", __func__);

	memcpy(&uuid, BT_UUID_BAS, sizeof(uuid));
	discover_params.uuid = &uuid.uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		FAIL("Discover failed(err %d)\n", err);
		return;
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

	if (conn != default_conn) {
		return;
	}

	discover_bas_service(conn);
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int i;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			FAIL("AD malformed\n");
			return true;
		}

		for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			const struct bt_uuid *uuid;
			struct bt_le_conn_param *param;
			uint16_t u16;
			int err;

			memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			if (bt_uuid_cmp(uuid, BT_UUID_BAS)) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err) {
				FAIL("Stop LE scan failed (err %d)\n", err);
				continue;
			}

			param = BT_LE_CONN_PARAM_DEFAULT;
			err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
			if (err) {
				printk("Create conn failed (err %d)\n", err);
			}

			return false;
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n", dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND || type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	printk("Scanning successfully re-started\n");
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_bas_central_main(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);

	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);

	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static const struct bst_test_instance test_bas_central[] = {
	{.test_id = "central",
	 .test_descr = "Battery Service test. It expects that a peripheral device can be found. "
		       "The test will pass if it can receive notifications and indications more "
		       "than the threshold set within 15 sec. ",
	 .test_pre_init_f = test_bas_central_init,
	 .test_tick_f = test_bas_central_tick,
	 .test_main_f = test_bas_central_main},
	BSTEST_END_MARKER};

struct bst_test_list *test_bas_central_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_bas_central);
	return tests;
}
