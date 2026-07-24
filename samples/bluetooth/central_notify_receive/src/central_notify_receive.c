/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

/* Interval between printing throughput metrics */
#define METRICS_INTERVAL 1U /* seconds */

/* Custom service UUID: 12345678-1234-5678-1234-56789abcdef0
 * This is used by peripheral_gatt_notify and for central role of multirole.
 */
#define BT_UUID_NOTIFY_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_NOTIFY_SERVICE BT_UUID_DECLARE_128(BT_UUID_NOTIFY_SERVICE_VAL)

/* Custom characteristic UUID: 12345678-1234-5678-1234-56789abcdef1 */
#define BT_UUID_NOTIFY_CHAR_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)
#define BT_UUID_NOTIFY_CHAR BT_UUID_DECLARE_128(BT_UUID_NOTIFY_CHAR_VAL)

/* Custom service UUID for multirole's peripheral role: 12345678-1234-5678-1234-56789abcdef2
 * This is what central_notify_receive discovers when connecting to the multirole device.
 */
#define BT_UUID_MULTIROLE_NOTIFY_CHAR_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)
#define BT_UUID_MULTIROLE_NOTIFY_CHAR \
	BT_UUID_DECLARE_128(BT_UUID_MULTIROLE_NOTIFY_CHAR_VAL)

static struct bt_conn *central_conn_connected;
static struct bt_gatt_subscribe_params central_subscribe_params;
static bool central_subscribed;
static bool central_role_active;
static void (*central_restart_scan_func)(void);

/* Throughput metrics */
static uint32_t central_notify_count;
static uint32_t central_notify_len;
static uint32_t central_notify_rate;
uint32_t central_last_notify_rate;
uint32_t *central_notify_countdown;

static uint8_t central_notify_cb(struct bt_conn *conn,
			  struct bt_gatt_subscribe_params *params,
			  const void *data, uint16_t length)
{
	static uint32_t cycle_stamp;
	uint64_t delta;

	if (!data) {
		printk("[Central] Notification subscription ended\n");
		params->value_handle = 0U;
		central_subscribed = false;
		return BT_GATT_ITER_STOP;
	}

	delta = k_cycle_get_32() - cycle_stamp;
	delta = k_cyc_to_ns_floor64(delta);

	if (delta == 0) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (delta > (METRICS_INTERVAL * NSEC_PER_SEC)) {
		printk("[Central] Notify RX: count= %u, len= %u, rate= %u bps.\n",
		       central_notify_count, central_notify_len, central_notify_rate);

		central_last_notify_rate = central_notify_rate;

		central_notify_count = 0U;
		central_notify_len = 0U;
		central_notify_rate = 0U;
		cycle_stamp = k_cycle_get_32();
	} else {
		central_notify_count++;
		central_notify_len += length;
		central_notify_rate = ((uint64_t)central_notify_len << 3) *
			      (METRICS_INTERVAL * NSEC_PER_SEC) / delta;
	}

	if (central_notify_countdown && *central_notify_countdown) {
		(*central_notify_countdown)--;
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t central_discover_cb(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("[Central] Discover complete\n");
		return BT_GATT_ITER_STOP;
	}

	printk("[Central] Discovered attr handle %u\n", attr->handle);

	/* Subscribe to notifications on the CCC (handle after char value) */
	central_subscribe_params.notify = central_notify_cb;
	central_subscribe_params.value = BT_GATT_CCC_NOTIFY;
	central_subscribe_params.value_handle = attr->handle + 1;
	central_subscribe_params.ccc_handle = attr->handle + 2;

	err = bt_gatt_subscribe(conn, &central_subscribe_params);
	if (err && err != -EALREADY) {
		printk("[Central] Subscribe failed (err %d)\n", err);
	} else {
		printk("[Central] Subscribed to notifications\n");
		central_subscribed = true;
	}

	return BT_GATT_ITER_STOP;
}

static struct bt_gatt_discover_params central_discover_params;
static struct bt_uuid_128 discover_uuid = BT_UUID_INIT_128(0);

static void central_discover_and_subscribe(struct bt_conn *conn)
{
	int err;

	printk("[Central] Discover ...\n");
	memcpy(&discover_uuid, BT_UUID_MULTIROLE_NOTIFY_CHAR, sizeof(discover_uuid));
	central_discover_params.uuid = &discover_uuid.uuid;
	central_discover_params.func = central_discover_cb;
	central_discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	central_discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	central_discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(conn, &central_discover_params);
	if (err) {
		printk("[Central] Discover failed (err %d)\n", err);
	}
}

static void central_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			  struct net_buf_simple *ad)
{
	struct bt_conn *conn;
	int err;

	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	/* Connect only to devices in close proximity */
	if (rssi < -50) {
		return;
	}

	err = bt_le_scan_stop();
	if (err) {
		printk("[Central] Stop LE scan failed (err %d)\n", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &conn);
	if (err) {
		printk("[Central] Create conn failed (err %d)\n", err);
		central_restart_scan_func();
	} else {
		bt_conn_unref(conn);
	}
}

static void central_start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, central_device_found);
	if (err) {
		printk("[Central] Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("[Central] Scanning successfully started\n");
}

static void central_mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_exchange_params *params)
{
	printk("[Central] MTU exchange %s (%u)\n",
	       err == 0U ? "successful" : "failed",
	       bt_gatt_get_mtu(conn));

	/* After MTU exchange, discover and subscribe */
	central_discover_and_subscribe(conn);
}

static struct bt_gatt_exchange_params central_mtu_exchange_params;

static void central_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("[Central] Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb central_gatt_callbacks = {
	.att_mtu_updated = central_mtu_updated,
};

static void central_connected(struct bt_conn *conn, uint8_t conn_err)
{
	if (!central_role_active) {
		return;
	}

	if (conn_err) {
		printk("[Central] Failed to connect (0x%02x)\n", conn_err);
		central_restart_scan_func();
		return;
	}

	printk("[Central] Connected: %s\n", bt_conn_dst_str(conn));
	central_conn_connected = bt_conn_ref(conn);

	/* Exchange MTU first, then discover */
	central_mtu_exchange_params.func = central_mtu_exchange_cb;
	int err = bt_gatt_exchange_mtu(conn, &central_mtu_exchange_params);

	if (err) {
		printk("[Central] MTU exchange failed (err %d)\n", err);
		/* Try discovery anyway */
		central_discover_and_subscribe(conn);
	}
}

static void central_disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != central_conn_connected) {
		return;
	}

	printk("[Central] Disconnected: %s (reason 0x%02x)\n", bt_conn_dst_str(conn), reason);

	if (central_conn_connected) {
		bt_conn_unref(central_conn_connected);
		central_conn_connected = NULL;
	}

	central_subscribed = false;

	central_restart_scan_func();
}

static void central_le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("[Central] Conn params updated: int 0x%04x lat %u to %u\n", interval,
	       latency, timeout);
}

BT_CONN_CB_DEFINE(central_recv_conn_cb) = {
	.connected = central_connected,
	.disconnected = central_disconnected,
	.le_param_updated = central_le_param_updated,
};

uint32_t central_notify_receive(uint32_t count)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("[Central] Bluetooth init failed (err %d)\n", err);
		return 0U;
	}
	printk("[Central] Bluetooth initialized\n");

	bt_gatt_cb_register(&central_gatt_callbacks);

	central_role_active = true;
	central_conn_connected = NULL;
	central_last_notify_rate = 0U;
	central_notify_countdown = &count;

	if (count != 0U) {
		printk("[Central] GATT Notify receive countdown %u.\n", count);
	} else {
		printk("[Central] GATT Notify receive forever.\n");
	}

	central_restart_scan_func = central_start_scan;
	central_start_scan();

	while (true) {
		if (count && !*central_notify_countdown) {
			break;
		}
		k_sleep(K_SECONDS(1));
	}

	return central_last_notify_rate;
}
