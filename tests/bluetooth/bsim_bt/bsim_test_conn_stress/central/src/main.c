/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#define TERM_PRINT(fmt, ...)   printk("\e[39m[Central] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_INFO(fmt, ...)    printk("\e[94m[Central] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_SUCCESS(fmt, ...) printk("\e[92m[Central] : " fmt "\e[39m\n", ##__VA_ARGS__)
#define TERM_ERR(fmt, ...)                                                                         \
	printk("\e[91m[Central] %s:%d : " fmt "\e[39m\n", __func__, __LINE__, ##__VA_ARGS__)
#define TERM_WARN(fmt, ...)                                                                        \
	printk("\e[93m[Central] %s:%d : " fmt "\e[39m\n", __func__, __LINE__, ##__VA_ARGS__)

#define PERIPHERAL_DEVICE_NAME	   "Zephyr Peripheral"
#define PERIPHERAL_DEVICE_NAME_LEN (sizeof(PERIPHERAL_DEVICE_NAME) - 1)

#define NOTIFICATION_DATA_PREFIX     "Counter:"
#define NOTIFICATION_DATA_PREFIX_LEN (sizeof(NOTIFICATION_DATA_PREFIX) - 1)

#define CHARACTERISTIC_DATA_MAX_LEN 260
#define NOTIFICATION_DATA_LEN	    (CONFIG_BT_L2CAP_TX_MTU - 4)
BUILD_ASSERT(NOTIFICATION_DATA_LEN <= CHARACTERISTIC_DATA_MAX_LEN);

#define PERIPHERAL_SERVICE_UUID_VAL                                                                \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define PERIPHERAL_CHARACTERISTIC_UUID_VAL                                                         \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

#define PERIPHERAL_SERVICE_UUID	       BT_UUID_DECLARE_128(PERIPHERAL_SERVICE_UUID_VAL)
#define PERIPHERAL_CHARACTERISTIC_UUID BT_UUID_DECLARE_128(PERIPHERAL_CHARACTERISTIC_UUID_VAL)

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
	CONN_INFO_MTU_EXCHANGED,
	CONN_INFO_SUBSCRIBED_TO_SERVICE,
	/* Total number of flags - must be at the end of the enum */
	CONN_INFO_NUM_FLAGS,
};

ATOMIC_DEFINE(status_flags, BT_IS_NUM_FLAGS);
static uint8_t volatile conn_count;
static struct bt_conn *conn_connecting;
static struct bt_gatt_exchange_params mtu_exchange_params;

struct conn_info {
	ATOMIC_DEFINE(flags, CONN_INFO_NUM_FLAGS);
	struct bt_conn *conn_ref;
	uint32_t notify_counter;
};

static struct conn_info conn_infos[CONFIG_BT_MAX_CONN];
static struct bt_uuid_128 uuid = BT_UUID_INIT_128(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

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

static bool check_if_peer_connected(const bt_addr_le_t *addr)
{

	for (size_t i = 0; i < ARRAY_SIZE(conn_infos); i++) {
		if (conn_infos[i].conn_ref != NULL) {
			if (!bt_addr_le_cmp(bt_conn_get_dst(conn_infos[i].conn_ref), addr)) {
				return true;
			}
		}
	}

	return false;
}

static bool check_all_flags_set(int bit)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_infos); i++) {
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
	CHECKIF(conn_info_ref == NULL) {
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

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	const char *data_ptr = (const char *)data + NOTIFICATION_DATA_PREFIX_LEN;
	uint32_t received_counter;
	struct conn_info *conn_info_ref;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!data) {
		TERM_INFO("[UNSUBSCRIBED] addr %s", addr);
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	conn_info_ref = get_conn_info_ref(conn);
	CHECKIF(conn_info_ref == NULL) {
		TERM_WARN("Invalid reference returned");
		return BT_GATT_ITER_CONTINUE;
	}

	received_counter = strtoul(data_ptr, NULL, 0);

	if ((conn_info_ref->notify_counter % 30) == 0) {
		TERM_PRINT("[NOTIFICATION] addr %s conn %u data %s length %u cnt %u", addr,
			   (conn_info_ref - conn_infos), data, length, received_counter);
	}

	__ASSERT(conn_info_ref->notify_counter == received_counter,
		 "expected counter : %u , received counter : %u", conn_info_ref->notify_counter,
		 received_counter);
	conn_info_ref->notify_counter++;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;
	char uuid_str[BT_UUID_STR_LEN];

	if (!attr) {
		TERM_INFO("Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	bt_uuid_to_str(params->uuid, uuid_str, sizeof(uuid_str));
	TERM_PRINT("UUID found : %s", uuid_str);

	TERM_PRINT("[ATTRIBUTE] handle %u", attr->handle);

	if (discover_params.type == BT_GATT_DISCOVER_PRIMARY) {
		TERM_PRINT("Primary Service Found");
		memcpy(&uuid, PERIPHERAL_CHARACTERISTIC_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			TERM_ERR("Discover failed (err %d)", err);
		}
	} else if (discover_params.type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		TERM_PRINT("Service Characteristic Found");
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			TERM_ERR("Discover failed (err %d)", err);
		}
	} else {
		subscribe_params.notify = notify_func;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			TERM_ERR("Subscribe failed (err %d)", err);
		} else {
			struct conn_info *conn_info_ref;
			char addr[BT_ADDR_LE_STR_LEN];

			conn_info_ref = get_conn_info_ref(conn);
			CHECKIF(conn_info_ref == NULL) {
				TERM_WARN("Invalid reference returned");
				return BT_GATT_ITER_STOP;
			}

			bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

			atomic_set_bit(conn_info_ref->flags, CONN_INFO_SUBSCRIBED_TO_SERVICE);
			TERM_INFO("[SUBSCRIBED] addr %s conn %u", addr,
				  (conn_info_ref - conn_infos));
		}
	}

	return BT_GATT_ITER_STOP;
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

			if (check_if_peer_connected(addr) == true) {
				TERM_ERR("Peer is already connected or in disconnecting state");
				break;
			}

			CHECKIF(atomic_test_and_set_bit(status_flags, BT_IS_CONNECTING) == true) {
				TERM_ERR("A connecting procedure is ongoing");
				break;
			}

			if (stop_scan()) {
				atomic_clear_bit(status_flags, BT_IS_CONNECTING);
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
	TERM_INFO("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i", dev, type, ad->len,
		  rssi);
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
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
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
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	TERM_PRINT("LE conn param req: %s int (0x%04x (~%u ms), 0x%04x (~%u ms)) lat %d to %d",
		   addr, param->interval_min, (uint32_t)(param->interval_min * 1.25),
		   param->interval_max, (uint32_t)(param->interval_max * 1.25), param->latency,
		   param->timeout);

	send_update_conn_params_req(conn);

	/* Reject the current connection parameters request */
	return false;
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

	if (conn == conn_connecting) {
		conn_connecting = NULL;
		atomic_clear_bit(status_flags, BT_IS_CONNECTING);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	TERM_INFO("Updated MTU: TX: %d RX: %d bytes", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {.att_mtu_updated = mtu_updated};

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	int conn_ref_index;
	struct conn_info *conn_info_ref;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	conn_info_ref = get_conn_info_ref(conn);
	CHECKIF(conn_info_ref == NULL) {
		TERM_WARN("Invalid reference returned");
		return;
	}

	conn_ref_index = (conn_info_ref - conn_infos);

	TERM_PRINT("MTU exchange addr %s conn %u %s", addr, conn_ref_index,
		   err == 0U ? "successful" : "failed");

	atomic_set_bit(conn_info_ref->flags, CONN_INFO_MTU_EXCHANGED);
}

static void exchange_mtu(struct bt_conn *conn, void *data)
{
	int conn_ref_index;
	struct conn_info *conn_info_ref;

	conn_info_ref = get_conn_info_ref(conn);
	CHECKIF(conn_info_ref == NULL) {
		TERM_WARN("Invalid reference returned");
		return;
	}

	conn_ref_index = (conn_info_ref - conn_infos);

	if (atomic_test_bit(conn_info_ref->flags, CONN_INFO_MTU_EXCHANGED) == false) {
		int err;
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		TERM_PRINT("MTU conn (%u): %u", conn_ref_index, bt_gatt_get_mtu(conn));
		TERM_PRINT("Updating MTU for %s...", addr);

		mtu_exchange_params.func = mtu_exchange_cb;

		err = bt_gatt_exchange_mtu(conn, &mtu_exchange_params);
		if (err) {
			TERM_ERR("MTU exchange failed (err %d)", err);
		} else {
			TERM_PRINT("Exchange pending...");
		}

		while (atomic_test_bit(conn_info_ref->flags, CONN_INFO_MTU_EXCHANGED) == false) {
			k_sleep(K_MSEC(10));
		}

		TERM_SUCCESS("Updating MTU succeeded %s", addr);
	}
}

static void subscribe_to_service(struct bt_conn *conn, void *data)
{
	struct conn_info *conn_info_ref;

	conn_info_ref = get_conn_info_ref(conn);
	CHECKIF(conn_info_ref == NULL) {
		TERM_WARN("Invalid reference returned");
		return;
	}

	if (atomic_test_bit(conn_info_ref->flags, CONN_INFO_SUBSCRIBED_TO_SERVICE) == false) {
		int err;
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		memcpy(&uuid, PERIPHERAL_SERVICE_UUID, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			TERM_ERR("Discover failed(err %d)", err);
			return;
		}

		while (atomic_test_bit(conn_info_ref->flags, CONN_INFO_SUBSCRIBED_TO_SERVICE) ==
		       false) {
			k_sleep(K_MSEC(10));
		}
	}
}

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

	bt_gatt_cb_register(&gatt_callbacks);

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
			k_sleep(K_MSEC(10));
			continue;
		}

		bt_conn_foreach(BT_CONN_TYPE_LE, exchange_mtu, NULL);

		bt_conn_foreach(BT_CONN_TYPE_LE, subscribe_to_service, NULL);

		while (conn_count == CONFIG_BT_MAX_CONN) {
			k_sleep(K_SECONDS(1));
		}
	}
}
