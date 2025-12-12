/** @file
 *  @brief Test local GATT Generic Access Service - central role
 *
 *  @note Most of the original code from "../device_name/client.c" used here.
 */
/*
 * Copyright (c) 2025 Koppel Electronic
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

#include "testlib/att.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"

#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(central, LOG_LEVEL_DBG);

/* Wait time in microseconds for the test to be finished */
#define WAIT_TIME 10e6

static struct bt_conn *default_conn;
static struct bt_conn *connected_conn;
struct k_sem connected_sem;

static void start_scan(void);


static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			printk("AD malformed\n");
			return true;
		}

		for (int i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			struct bt_conn_le_create_param *create_param;
			struct bt_le_conn_param *param;
			const struct bt_uuid *uuid;
			uint16_t u16;
			int err;

			memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			if (bt_uuid_cmp(uuid, BT_UUID_HRS)) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err) {
				printk("Stop LE scan failed (err %d)\n", err);
				continue;
			}

			printk("Creating connection with Coded PHY support\n");
			param = BT_LE_CONN_PARAM_DEFAULT;
			create_param = BT_CONN_LE_CREATE_CONN;
			create_param->options |= BT_CONN_LE_OPT_CODED;
			err = bt_conn_le_create(addr, create_param, param,
						&default_conn);
			if (err) {
				printk("Create connection with Coded PHY support failed (err %d)\n",
				       err);

				printk("Creating non-Coded PHY connection\n");
				create_param->options &= ~BT_CONN_LE_OPT_CODED;
				err = bt_conn_le_create(addr, create_param,
							param, &default_conn);
				if (err) {
					printk("Create connection failed (err %d)\n", err);
					start_scan();
				}
			}

			return false;
		}
	default:
		break;
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

	/* We're only interested in legacy connectable events or
	 * possible extended advertising that are connectable.
	 */
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
	    type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND ||
	    type == BT_GAP_ADV_TYPE_EXT_ADV) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static void start_scan(void)
{
	int err;

	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime.
	 */
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_CODED,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Scanning with Coded PHY support failed (err %d)\n", err);

		printk("Scanning without Coded PHY\n");
		scan_param.options &= ~BT_LE_SCAN_OPT_CODED;
		err = bt_le_scan_start(&scan_param, device_found);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return;
		}
	}

	printk("Scanning successfully started\n");
}


static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	printk("Connected: %s\n", addr);

	if (conn == default_conn) {
		connected_conn = bt_conn_ref(conn);
		k_sem_give(&connected_sem);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	if (default_conn != conn) {
		return;
	}

	struct bt_conn *conn_to_unref;

	conn_to_unref = connected_conn;
	connected_conn = NULL;
	bt_conn_unref(conn_to_unref);

	conn_to_unref = default_conn;
	default_conn = NULL;
	bt_conn_unref(conn_to_unref);

	start_scan();
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_gap_name(struct bt_conn *conn)
{
	int err;
	char server_new_name[CONFIG_BT_DEVICE_NAME_MAX] = CONFIG_BT_DEVICE_NAME"-up";
	uint16_t chrc_handle;

	NET_BUF_SIMPLE_DEFINE(attr_value_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

	err = bt_testlib_gatt_discover_characteristic(&chrc_handle,
						      NULL, NULL, conn,
						      BT_UUID_GAP_DEVICE_NAME,
						      BT_ATT_FIRST_ATTRIBUTE_HANDLE,
						      BT_ATT_LAST_ATTRIBUTE_HANDLE);
	TEST_ASSERT(err == 0, "Device Name characteristic not found (err %d)", err);

	LOG_DBG("Device Name characteristic found at handle %u", chrc_handle);

	/* Read Device name */
	err = bt_testlib_att_read_by_handle_sync(&attr_value_buf, NULL, NULL, conn,
						BT_ATT_CHAN_OPT_UNENHANCED_ONLY, chrc_handle, 0);
	TEST_ASSERT(err == 0, "Failed to read characteristic (err %d)", err);

	LOG_DBG("Device Name of the server: %.*s", attr_value_buf.len, attr_value_buf.data);

	net_buf_simple_reset(&attr_value_buf);

	/* Write new Device name */
	err = bt_testlib_att_write(conn,
				   BT_ATT_CHAN_OPT_UNENHANCED_ONLY,
				   chrc_handle,
				   server_new_name,
				   sizeof(server_new_name));
	TEST_ASSERT(err == BT_ATT_ERR_SUCCESS, "Got ATT error: %d", err);

	/* Verify new Device name */
	err = bt_testlib_att_read_by_handle_sync(&attr_value_buf, NULL, NULL, conn,
						BT_ATT_CHAN_OPT_UNENHANCED_ONLY, chrc_handle, 0);
	TEST_ASSERT(err == 0, "Failed to read characteristic (err %d)", err);

	TEST_ASSERT(attr_value_buf.len == strlen(server_new_name),
			"Unexpected Device Name length: %u (!=%u)",
			attr_value_buf.len, strlen(server_new_name));
	TEST_ASSERT(memcmp(attr_value_buf.data, server_new_name, attr_value_buf.len) == 0,
			"Unexpected Device Name value: %.*s",
			attr_value_buf.len, attr_value_buf.data);

	net_buf_simple_reset(&attr_value_buf);
}

static void test_gap_appearance(struct bt_conn *conn)
{
	int err;
	uint16_t chrc_handle;
	uint16_t appearance;

	NET_BUF_SIMPLE_DEFINE(attr_value_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

	err = bt_testlib_gatt_discover_characteristic(&chrc_handle,
						      NULL, NULL, conn,
						      BT_UUID_GAP_APPEARANCE,
						      BT_ATT_FIRST_ATTRIBUTE_HANDLE,
						      BT_ATT_LAST_ATTRIBUTE_HANDLE);
	TEST_ASSERT(err == 0, "Device Appearance characteristic not found (err %d)", err);

	LOG_DBG("Device Appearance characteristic found at handle %u", chrc_handle);

	/* Read Device appearance */
	err = bt_testlib_att_read_by_handle_sync(&attr_value_buf, NULL, NULL, conn,
						BT_ATT_CHAN_OPT_UNENHANCED_ONLY, chrc_handle, 0);
	TEST_ASSERT(err == 0, "Failed to read characteristic (err %d)", err);
	TEST_ASSERT(attr_value_buf.len == sizeof(appearance),
			"Unexpected Appearance length: %u (!=%u)",
			attr_value_buf.len, sizeof(uint16_t));
	appearance = sys_le16_to_cpu(*(uint16_t *)attr_value_buf.data);
	LOG_DBG("Device Appearance of the server: %.4x", appearance);
	net_buf_simple_reset(&attr_value_buf);

	/* Write new Device appearance */
	appearance += 0x100;
	err = bt_testlib_att_write(conn, BT_ATT_CHAN_OPT_UNENHANCED_ONLY, chrc_handle,
				   (char *)&appearance, sizeof(appearance));
	TEST_ASSERT(err == BT_ATT_ERR_SUCCESS, "Got ATT error: %d", err);
}

static void test_local_gap_svc_central_main(void)
{
	int err;
	struct bt_conn *conn;

	k_sem_init(&connected_sem, 0, 1);
	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Cannot enable Bluetooth (err %d)", err);

	LOG_INF("Bluetooth initialized");

	start_scan();

	/* Wait for connection */
	k_sem_take(&connected_sem, K_FOREVER);

	conn = bt_conn_ref(connected_conn);

	err = bt_testlib_att_exchange_mtu(conn);
	TEST_ASSERT(err == 0, "Failed to update MTU (err %d)", err);

	test_gap_name(conn);
	test_gap_appearance(conn);

	bt_conn_unref(conn);

	TEST_PASS("client");
}

static void test_local_gap_svc_central_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	TEST_START("test_local_gap_svc_central");
}

static void test_local_gap_svc_central_tick(bs_time_t HW_device_time)
{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		TEST_FAIL("test_local_gap_svc_central failed (not passed after %d seconds)",
		     (int)(WAIT_TIME / 1e6));
	}
}



static const struct bst_test_instance test_central[] = {
	{
		.test_id = "central",
		.test_descr = "GAP service local reimplementation - central role.",
		.test_main_f = test_local_gap_svc_central_main,
		.test_pre_init_f = test_local_gap_svc_central_init,
		.test_tick_f = test_local_gap_svc_central_tick,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_local_gap_svc_central_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_central);
	return tests;
}
