/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>

#define PERIPHERAL_DEVICE_NAME "Zephyr Peripheral"
#define PERIPHERAL_DEVICE_NAME_LEN (sizeof(PERIPHERAL_DEVICE_NAME) - 1)

#define TERM_PRINT(fmt, ...)   printk("\e[39m[Central] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_INFO(fmt, ...)    printk("\e[94m[Central] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_SUCCESS(fmt, ...) printk("\e[92m[Central] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_ERR(fmt, ...)                                                                         \
	printk("\e[91m[Central] %s:%d : " fmt "\e[39m\n", __func__, __LINE__, ##__VA_ARGS__)
#define TERM_WARN(fmt, ...)                                                                        \
	printk("\e[93m[Central] %s:%d : " fmt "\e[39m\n", __func__, __LINE__, ##__VA_ARGS__)

static void start_scan(void);
static int stop_scan(void);

enum {
	BT_IS_SCANNING,
	BT_IS_CONNECTING,
	/* Total number of flags - must be at the end of the enum */
	BT_IS_NUM_FLAGS,
};

ATOMIC_DEFINE(status_flags, BT_IS_NUM_FLAGS);
static uint8_t volatile conn_count;
static struct bt_conn *conn_connecting;
static struct bt_conn *conn_refs[CONFIG_BT_MAX_CONN];

static int get_empty_conn_ref_index(struct bt_conn *conn_ref)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_refs); i++) {
		if (conn_refs[i] == NULL) {
			return i;
		}
	}

	return -1;
}

static int get_conn_ref_index(struct bt_conn *conn_ref)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_refs); i++) {
		if (conn_ref == conn_refs[i]) {
			return i;
		}
	}

	return -1;
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;

	TERM_PRINT("[AD]: %u data_len %u", data->type, data->data_len);

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		TERM_PRINT("Name Tag found!");
		TERM_PRINT("Device name : %.*s", data->data_len, data->data);

		if (!strncmp(data->data, PERIPHERAL_DEVICE_NAME, PERIPHERAL_DEVICE_NAME_LEN)) {
			int err;
			char dev[BT_ADDR_LE_STR_LEN];
			struct bt_le_conn_param *param;

			if (stop_scan()) {
				break;
			}

			CHECKIF(atomic_test_and_set_bit(status_flags, BT_IS_CONNECTING) == true) {
				TERM_ERR("A connecting procedure is ongoing");
				break;
			}

			param = BT_LE_CONN_PARAM_DEFAULT;

			bt_addr_le_to_str(addr, dev, sizeof(dev));
			TERM_INFO("Connecting to %s", dev);
			err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param,
						&conn_connecting);
			if (err) {
				TERM_ERR("Create conn failed (err %d)", err);
				atomic_clear_bit(status_flags, BT_IS_CONNECTING);
			}

			return false;
		}

		break;
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	TERM_PRINT("------------------------------------------------------");
	TERM_INFO("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i",
	       dev, type, ad->len, rssi);
	TERM_PRINT("------------------------------------------------------");

	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
	    type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND ||
		type == BT_GAP_ADV_TYPE_SCAN_RSP) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static void start_scan(void)
{
	int err;

	CHECKIF(atomic_test_and_set_bit(status_flags, BT_IS_SCANNING) == true) {
		TERM_ERR("A scanning procedure is ongoing");
		return;
	}

	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime.
	 */
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		TERM_ERR("Scanning failed to start (err %d)", err);
		atomic_clear_bit(status_flags, BT_IS_SCANNING);
		return;
	}

	TERM_INFO("Scanning successfully started");
}

static int stop_scan(void)
{
	int err;

	CHECKIF(atomic_test_bit(status_flags, BT_IS_SCANNING) == false) {
		TERM_ERR("No scanning procedure is ongoing");
		return -EALREADY;
	}

	err = bt_le_scan_stop();
	if (err) {
		TERM_ERR("Stop LE scan failed (err %d)", err);
		return err;
	}

	atomic_clear_bit(status_flags, BT_IS_SCANNING);
	TERM_INFO("Scanning successfully stopped");
	return 0;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	int conn_ref_index;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		TERM_ERR("Failed to connect to %s (%u)", addr, conn_err);

		bt_conn_unref(conn_connecting);
		conn_connecting = NULL;
		atomic_clear_bit(status_flags, BT_IS_CONNECTING);

		return;
	}

	TERM_SUCCESS("Connection %p established : %s", conn, addr);

	conn_count++;
	TERM_INFO("Active connections count : %u", conn_count);

	conn_ref_index = get_empty_conn_ref_index(conn);
	if (conn_ref_index < 0) {
		TERM_WARN("Invalid connection reference index returned");
		return;
	}
	conn_refs[conn_ref_index] = conn_connecting;

#if defined(CONFIG_BT_SMP)
	err = bt_conn_set_security(conn, BT_SECURITY_L2);

	if (!err) {
		TERM_SUCCESS("Security level is set to : %u", BT_SECURITY_L2);
	} else {
		TERM_ERR("Failed to set security (%d).", err);
	}

	if (conn == conn_connecting) {
		conn_connecting = NULL;
		atomic_clear_bit(status_flags, BT_IS_CONNECTING);

		if (conn_count < CONFIG_BT_MAX_CONN) {
			start_scan();
		}
	}
#endif
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int conn_ref_index;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_ERR("Disconnected: %s (reason 0x%02x)", addr, reason);

	conn_ref_index = get_conn_ref_index(conn);
	if (conn_ref_index < 0) {
		TERM_WARN("Invalid connection reference index returned");
		return;
	}

	bt_conn_unref(conn_refs[conn_ref_index]);
	conn_refs[conn_ref_index] = NULL;
	conn_count--;
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		TERM_INFO("Security for %p changed: %s level %u", conn, addr, level);
	} else {
		TERM_ERR("Security for %p failed: %s level %u err %d", conn, addr, level, err);
	}
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

void main(void)
{
	int err;

	err = bt_enable(NULL);

	if (err) {
		TERM_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	TERM_PRINT("Bluetooth initialized");

	start_scan();
}
