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
#include <zephyr/sys/byteorder.h>

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

enum {
	CONN_INFO_CONN_PARAMS_UPDATED,
	CONN_INFO_LL_DATA_LEN_UPDATED,
	/* Total number of flags - must be at the end of the enum */
	CONN_INFO_NUM_FLAGS,
};

ATOMIC_DEFINE(status_flags, BT_IS_NUM_FLAGS);
static uint8_t volatile conn_count;
static struct bt_conn *conn_connecting;

struct conn_info {
	ATOMIC_DEFINE(flags, CONN_INFO_NUM_FLAGS);
	struct bt_conn *conn_ref;
};

static struct conn_info conn_infos[CONFIG_BT_MAX_CONN];

static struct conn_info *get_new_conn_info_ref(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_infos); i++) {
		if (conn_infos[i].conn_ref == NULL) {
			return &conn_infos[i];
		}
	}

	return NULL;
}

static struct conn_info *get_conn_info_ref(struct bt_conn *conn_ref)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_infos); i++) {
		if (conn_ref == conn_infos[i].conn_ref) {
			return &conn_infos[i];
		}
	}

	return NULL;
}

static bool check_all_flags_set(int bit)
{
	TERM_INFO("---> Checking flags :");
	for (size_t i = 0; i < ARRAY_SIZE(conn_infos); i++) {
		TERM_INFO("\t---> %d - %s\e[39m <---", i + 1,
			  atomic_test_bit(conn_infos[i].flags, bit) ? "\e[92mOK" : "\e[91mErr");
		if (atomic_test_bit(conn_infos[i].flags, bit) == false) {
			return false;
		}
	}

	return true;
}

static void send_update_conn_params_req(struct bt_conn *conn)
{
	struct conn_info *conn_info_ref;

	conn_info_ref = get_conn_info_ref(conn);
	CHECKIF(conn_info_ref == NULL)
	{
		TERM_WARN("Invalid reference returned");
		return;
	}

	if (atomic_test_bit(conn_info_ref->flags, CONN_INFO_CONN_PARAMS_UPDATED) == false) {
		int err;
		char addr[BT_ADDR_LE_STR_LEN];
		/** Default LE connection parameters:
		 *    Connection Interval: 30-50 ms
		 *    Latency: 0
		 *    Timeout: 4 s
		 */
		struct bt_le_conn_param param = *BT_LE_CONN_PARAM_DEFAULT;

		param.interval_min = 10; /* (10 * 1.25 ms) = 12.5 */
		param.interval_max = 20; /* (20 * 1.25 ms) = 25 */

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		TERM_PRINT("Updating %s... connection parameters", addr);

		err = bt_conn_le_param_update(conn, &param);
		if (err) {
			TERM_ERR("Updating connection parameters failed %s", addr);
			return;
		}

		TERM_SUCCESS("Updating connection parameters succeeded %s", addr);
	}
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
	if (type == BT_GAP_ADV_TYPE_SCAN_RSP) {
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
	struct conn_info *conn_info_ref;
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

	conn_info_ref = get_new_conn_info_ref();
	CHECKIF(conn_info_ref == NULL) {
		TERM_WARN("Invalid reference returned");
		return;
	}
	TERM_PRINT("Connection reference store index %u", (conn_info_ref - conn_infos));
	conn_info_ref->conn_ref = conn_connecting;

	if (conn == conn_connecting) {
		conn_connecting = NULL;
		atomic_clear_bit(status_flags, BT_IS_CONNECTING);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct conn_info *conn_info_ref;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_ERR("Disconnected: %s (reason 0x%02x)", addr, reason);
	bt_conn_unref(conn);

	conn_info_ref = get_conn_info_ref(conn);
	CHECKIF(conn_info_ref == NULL) {
		TERM_WARN("Invalid reference returned");
		return;
	}
	TERM_PRINT("Connection reference store index %u", (conn_info_ref - conn_infos));

	conn_info_ref->conn_ref = NULL;
	memset(conn_info_ref, 0x00, sizeof(struct conn_info));

	conn_count--;
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct conn_info *conn_info_ref;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	conn_info_ref = get_conn_info_ref(conn);
	CHECKIF(conn_info_ref == NULL) {
		TERM_WARN("Invalid reference returned");
		return false;
	}

	TERM_PRINT("\e[91mLE conn param req: %s (%u) int (0x%04x, 0x%04x) lat %d to %d", addr,
		   (conn_info_ref - conn_infos), param->interval_min, param->interval_max,
		   param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	struct conn_info *conn_info_ref;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_INFO("LE conn param updated: %s int 0x%04x (~%u ms) lat %d to %d", addr, interval,
		  (uint32_t)(interval * 1.25), latency, timeout);

	conn_info_ref = get_conn_info_ref(conn);
	CHECKIF(conn_info_ref == NULL) {
		TERM_WARN("Invalid reference returned");
		return;
	}

	atomic_set_bit(conn_info_ref->flags, CONN_INFO_CONN_PARAMS_UPDATED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

void main(void)
{
	int err;

	memset(&conn_infos, 0x00, sizeof(conn_infos));

	err = bt_enable(NULL);

	if (err) {
		TERM_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	TERM_PRINT("Bluetooth initialized");

	start_scan();

	while (true) {
		while (atomic_test_bit(status_flags, BT_IS_SCANNING) == true ||
		       atomic_test_bit(status_flags, BT_IS_CONNECTING) == true) {
			k_sleep(K_MSEC(10));
		}

		if (conn_count < CONFIG_BT_MAX_CONN) {
			start_scan();
			continue;
		}

		if (check_all_flags_set(CONN_INFO_CONN_PARAMS_UPDATED) == false) {
			k_sleep(K_SECONDS(100));
			TERM_ERR("---> Not all flags were set <---");
			continue;
		} else {
			TERM_SUCCESS("---> All flags were set <---");
		}

		while (true) {
			k_sleep(K_MSEC(10));
		}
	}
}
