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
static struct bt_gatt_subscribe_params subscribe_params;

#define UPDATE_PARAM_INTERVAL_MIN 25
#define UPDATE_PARAM_INTERVAL_MAX 45
#define UPDATE_PARAM_LATENCY      1
#define UPDATE_PARAM_TIMEOUT      250

static struct bt_le_conn_param update_params = {
	.interval_min = UPDATE_PARAM_INTERVAL_MIN,
	.interval_max = UPDATE_PARAM_INTERVAL_MAX,
	.latency = UPDATE_PARAM_LATENCY,
	.timeout = UPDATE_PARAM_TIMEOUT,
};

static bool encrypt_link;
static bool expect_ntf = true;
static uint8_t repeat_connect;
static uint8_t connected_signal;

/*
 * Basic connection test:
 *   We expect to find a connectable peripheral to which we will
 *   connect.
 *
 *   After connecting, we update connection parameters and channel
 *   map, and expect to receive 2 notifications.
 *   If we do, the test case passes.
 *   If we do not in 5 seconds, the testcase is considered failed
 *
 *   The thread code is mostly a copy of the central_hr sample device
 */

#define WAIT_TIME 6 /*seconds*/
#define WAIT_TIME_REPEAT 22 /*seconds*/
extern enum bst_result_t bst_result;

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

static void test_con1_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME*1e6);
	bst_result = In_progress;
}

static void test_con_encrypted_init(void)
{
	encrypt_link = true;
	test_con1_init();
}

static void test_con20_init(void)
{
	repeat_connect = 20;
	expect_ntf = false;
	bst_ticker_set_next_tick_absolute(WAIT_TIME_REPEAT*1e6);
	bst_result = In_progress;
}

static void test_con1_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		FAIL("test_connect1 failed (not passed after %i seconds)\n",
		     WAIT_TIME);
	}
}

static void test_con20_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("test_connect1 failed (not passed after %i seconds)\n",
		     WAIT_TIME_REPEAT);
	}
}

static uint8_t notify_func(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	static int notify_count;
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	printk("[NOTIFICATION] data %p length %u\n", data, length);

	if (notify_count++ >= 1) { /* We consider it passed */
		int err;

		/* Disconnect before actually passing */
		err = bt_conn_disconnect(default_conn,
					 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			FAIL("Disconnection failed (err %d)\n", err);
			return BT_GATT_ITER_STOP;
		}

		if (bst_result != Failed) {
			PASS("Testcase passed\n");
		}
		bs_trace_silent_exit(0);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
		const struct bt_gatt_attr *attr,
		struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HRS)) {
		memcpy(&uuid, BT_UUID_HRS_MEASUREMENT, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			FAIL("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid,
			BT_UUID_HRS_MEASUREMENT)) {
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = attr->handle + 1;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			FAIL("Discover failed (err %d)\n", err);
		}
	} else {
		subscribe_params.notify = notify_func;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			FAIL("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static void update_conn(struct bt_conn *conn, bool bonded)
{
	int err;

	if (encrypt_link != bonded) {
		FAIL("Unexpected bonding status\n");
		return;
	}

	printk("Updating connection (bonded: %d)\n", bonded);

	err = bt_conn_le_param_update(conn, &update_params);
	if (err) {
		FAIL("Parameter update failed (err %d)\n", err);
		return;
	}
}

static struct bt_conn_auth_info_cb auth_cb_success = {
	.pairing_complete = update_conn,
};

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

	if (conn != default_conn) {
		return;
	}

	if (encrypt_link) {
		k_sleep(K_MSEC(500));
		bt_conn_auth_info_cb_register(&auth_cb_success);
		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			FAIL("bt_conn_set_security failed (err %d)\n", err);
			return;
		}
	} else {
		update_conn(conn, false);
	}
}

static void params_updated(struct bt_conn *conn, uint16_t interval,
			   uint16_t latency, uint16_t timeout)
{
	uint8_t chm[5] = { 0x11, 0x22, 0x33, 0x44, 0x00 };
	int err;

	if (interval != UPDATE_PARAM_INTERVAL_MAX ||
	    latency != UPDATE_PARAM_LATENCY ||
	    timeout != UPDATE_PARAM_TIMEOUT) {
		FAIL("Unexpected connection parameters "
		     "(interval: %d, latency: %d, timeout: %d)\n",
		     interval, latency, timeout);
		return;
	}

	printk("Connection parameters updated "
	       "(interval: %d, latency: %d, timeout: %d)\n",
	       interval, latency, timeout);

	err = bt_le_set_chan_map(chm);
	if (err) {
		FAIL("Channel map update failed (err %d)\n", err);
		return;
	}

	if (!expect_ntf) {
		connected_signal = 1;
	} else {
		memcpy(&uuid, BT_UUID_HRS, sizeof(uuid));
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
			if (bt_uuid_cmp(uuid, BT_UUID_HRS)) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err) {
				FAIL("Stop LE scan failed (err %d)\n", err);
				continue;
			}

			param = BT_LE_CONN_PARAM_DEFAULT;
			err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
						param, &default_conn);
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
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
			dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
	    type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
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
	.le_param_updated = params_updated,
};

static void test_con1_main(void)
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

static void test_con20_main(void)
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
	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		k_sleep(K_MSEC(500));

		if (connected_signal) {
			/* Disconnect and continue */
			printk("Central Disconnect\n");
			connected_signal = 0;
			err = bt_conn_disconnect(default_conn,
						 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			if (err) {
				FAIL("Disconnection failed (err %d)\n", err);
				return;
			}

			if (bst_result != Failed) {
				if (repeat_connect) {
					printk("Disconnection OK\n");
				} else {
					PASS("Testcase passed\n");
				}
			}
			if (!repeat_connect || bst_result == Failed) {
				bs_trace_silent_exit(0);
			}
			repeat_connect--;
		}
	}

}

static const struct bst_test_instance test_connect[] = {
	{
		.test_id = "central",
		.test_descr = "Basic connection test. It expects that a "
			      "peripheral device can be found. The test will "
			      "pass if it can connect to it, and receive a "
			      "notification in less than 5 seconds.",
		.test_post_init_f = test_con1_init,
		.test_tick_f = test_con1_tick,
		.test_main_f = test_con1_main
	},
	{
		.test_id = "central_encrypted",
		.test_descr = "Same as central but with an encrypted link",
		.test_post_init_f = test_con_encrypted_init,
		.test_tick_f = test_con1_tick,
		.test_main_f = test_con1_main
	},
	{
		.test_id = "central_repeat20",
		.test_descr = "Multiple connections test. It expects that a "
			      "peripheral device can be found. The test will "
			      "pass if it can connect to it 20 times, in less than 22 seconds."
			      "Disconnect and re-connect 20 times",
		.test_post_init_f = test_con20_init,
		.test_tick_f = test_con20_tick,
		.test_main_f = test_con20_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_connect1_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_connect);
	return tests;
}
