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

/* Custom service UUID: 12345678-1234-5678-1234-56789abcdef0 */
#define BT_UUID_NOTIFY_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_NOTIFY_SERVICE BT_UUID_DECLARE_128(BT_UUID_NOTIFY_SERVICE_VAL)

/* Custom characteristic UUID in remote peripheral role: 12345678-1234-5678-1234-56789abcdef1 */
#define BT_UUID_NOTIFY_CHAR_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)
#define BT_UUID_NOTIFY_CHAR BT_UUID_DECLARE_128(BT_UUID_NOTIFY_CHAR_VAL)

/* Custom service UUID for multirole's peripheral role: 12345678-1234-5678-1234-56789abcdef2 */
#define BT_UUID_MULTIROLE_NOTIFY_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)
#define BT_UUID_MULTIROLE_NOTIFY_SERVICE \
	BT_UUID_DECLARE_128(BT_UUID_MULTIROLE_NOTIFY_SERVICE_VAL)

/* Custom characteristic UUID for multirole's peripheral role:
 *	12345678-1234-5678-1234-56789abcdef3
 */
#define BT_UUID_MULTIROLE_NOTIFY_CHAR_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)
#define BT_UUID_MULTIROLE_NOTIFY_CHAR \
	BT_UUID_DECLARE_128(BT_UUID_MULTIROLE_NOTIFY_CHAR_VAL)

/* ===================== Peripheral role (sends notifications to Device 1) ====================== */

static struct bt_conn *peripheral_conn;
static bool peripheral_notify_enabled;
static struct bt_conn *central_conn;
static bool central_subscribed;
static bool multirole_active;

/* Throughput metrics for sent notifications */
static uint32_t tx_notify_count;
static uint32_t tx_notify_len;
static uint32_t tx_notify_rate;
uint32_t multirole_last_tx_notify_rate;

static void multirole_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	peripheral_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	printk("[Multirole] [Peripheral] Notifications %s\n",
	       peripheral_notify_enabled ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(multirole_notify_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MULTIROLE_NOTIFY_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_MULTIROLE_NOTIFY_CHAR,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC(multirole_ccc_cfg_changed,
		     BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static const struct bt_data multirole_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static int multirole_send_notification(struct bt_conn *conn)
{
	static uint8_t data[BT_ATT_MAX_ATTRIBUTE_LEN];
	uint16_t data_len;
	int err;

	data_len = bt_gatt_get_mtu(conn) - 3;
	if (data_len > BT_ATT_MAX_ATTRIBUTE_LEN) {
		data_len = BT_ATT_MAX_ATTRIBUTE_LEN;
	}

	err = bt_gatt_notify(conn, &multirole_notify_svc.attrs[1], data, data_len);
	if (err == -ENOMEM) {
		return err;
	}

	if (err) {
		printk("[Multirole] [Peripheral] Notify failed (err %d)\n", err);
		return err;
	}

	return data_len;
}

static void update_tx_metrics(uint16_t len)
{
	static uint32_t cycle_stamp;
	uint64_t delta;

	delta = k_cycle_get_32() - cycle_stamp;
	delta = k_cyc_to_ns_floor64(delta);

	if (delta == 0) {
		return;
	}

	if (delta > (METRICS_INTERVAL * NSEC_PER_SEC)) {
		printk("[Multirole] [Peripheral] Notify TX: count= %u, len= %u, rate= %u bps.\n",
		       tx_notify_count, tx_notify_len, tx_notify_rate);

		multirole_last_tx_notify_rate = tx_notify_rate;

		tx_notify_count = 0U;
		tx_notify_len = 0U;
		tx_notify_rate = 0U;
		cycle_stamp = k_cycle_get_32();
	} else {
		tx_notify_count++;
		tx_notify_len += len;
		tx_notify_rate = ((uint64_t)tx_notify_len << 3) *
				 (METRICS_INTERVAL * NSEC_PER_SEC) / delta;
	}
}

/* ===================== Central role (receives notifications from Device 3) ==================== */

static struct bt_gatt_subscribe_params subscribe_params;

/* Throughput metrics for received notifications */
static uint32_t rx_notify_count;
static uint32_t rx_notify_len;
static uint32_t rx_notify_rate;
uint32_t multirole_last_rx_notify_rate;
uint32_t *multirole_notify_countdown;

static uint8_t notify_rx_cb(struct bt_conn *conn,
			     struct bt_gatt_subscribe_params *params,
			     const void *data, uint16_t length)
{
	static uint32_t cycle_stamp;
	uint64_t delta;

	if (!data) {
		printk("[Multirole] [Central] Notification subscription ended\n");
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
		printk("[Multirole] [Central] Notify RX: count= %u, len= %u, rate= %u bps.\n",
		       rx_notify_count, rx_notify_len, rx_notify_rate);

		multirole_last_rx_notify_rate = rx_notify_rate;

		rx_notify_count = 0U;
		rx_notify_len = 0U;
		rx_notify_rate = 0U;
		cycle_stamp = k_cycle_get_32();
	} else {
		rx_notify_count++;
		rx_notify_len += length;
		rx_notify_rate = ((uint64_t)rx_notify_len << 3) *
				 (METRICS_INTERVAL * NSEC_PER_SEC) / delta;
	}

	if (multirole_notify_countdown && *multirole_notify_countdown) {
		(*multirole_notify_countdown)--;
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_cb(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("[Multirole] [Central] Discover complete\n");
		return BT_GATT_ITER_STOP;
	}

	printk("[Multirole] [Central] Discovered attr handle %u\n", attr->handle);

	subscribe_params.notify = notify_rx_cb;
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	subscribe_params.value_handle = attr->handle + 1;
	subscribe_params.ccc_handle = attr->handle + 2;

	err = bt_gatt_subscribe(conn, &subscribe_params);
	if (err && err != -EALREADY) {
		printk("[Multirole] [Central] Subscribe failed (err %d)\n", err);
	} else {
		printk("[Multirole] [Central] Subscribed to notifications\n");
		central_subscribed = true;
	}

	return BT_GATT_ITER_STOP;
}

static struct bt_gatt_discover_params discover_params;
static struct bt_uuid_128 discover_uuid = BT_UUID_INIT_128(0);

static void discover_and_subscribe(struct bt_conn *conn)
{
	int err;

	printk("[Multirole] [Central] Discover ...\n");
	memcpy(&discover_uuid, BT_UUID_NOTIFY_CHAR, sizeof(discover_uuid));
	discover_params.uuid = &discover_uuid.uuid;
	discover_params.func = discover_cb;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		printk("[Multirole] [Central] Discover failed (err %d)\n", err);
	}
}

/* ===================== Scanning (central role) ===================== */

static void multirole_start_scan(void);

static void multirole_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
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
		printk("[Multirole] [Central] Stop LE scan failed (err %d)\n", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &conn);
	if (err) {
		printk("[Multirole] [Central] Create conn failed (err %d)\n", err);
		multirole_start_scan();
	} else {
		bt_conn_unref(conn);
	}
}

static void multirole_start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, multirole_device_found);
	if (err) {
		printk("[Multirole] [Central] Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("[Multirole] [Central] Scanning successfully started\n");
}

/* ===================== Connection callbacks ===================== */

static void multirole_mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_exchange_params *params)
{
	printk("[Multirole] MTU exchange %s (%u)\n",
	       err == 0U ? "successful" : "failed",
	       bt_gatt_get_mtu(conn));

	/* If this is the central connection, discover and subscribe */
	if (conn == central_conn) {
		discover_and_subscribe(conn);
	}
}

static struct bt_gatt_exchange_params multirole_mtu_exchange_params_central;
static struct bt_gatt_exchange_params multirole_mtu_exchange_params_peripheral;

static void multirole_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("[Multirole] Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb multirole_gatt_callbacks = {
	.att_mtu_updated = multirole_mtu_updated,
};

static void multirole_connected(struct bt_conn *conn, uint8_t conn_err)
{
	struct bt_conn_info conn_info;
	int err;

	if (!multirole_active) {
		return;
	}

	if (conn_err) {
		printk("[Multirole] Failed to connect (0x%02x)\n", conn_err);
		return;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("[Multirole] Failed to get connection info (%d)\n", err);
		return;
	}

	printk("[Multirole] Connected: %s role %u\n", bt_conn_dst_str(conn), conn_info.role);

	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		/* We connected as central to Device 3 (peripheral_gatt_notify) */
		central_conn = bt_conn_ref(conn);

		multirole_mtu_exchange_params_central.func = multirole_mtu_exchange_cb;
		err = bt_gatt_exchange_mtu(conn, &multirole_mtu_exchange_params_central);
		if (err) {
			printk("[Multirole] [Central] MTU exchange failed (err %d)\n", err);
			discover_and_subscribe(conn);
		}
	} else {
		/* Device 1 connected to us as peripheral */
		peripheral_conn = bt_conn_ref(conn);

		multirole_mtu_exchange_params_peripheral.func = multirole_mtu_exchange_cb;
		(void)bt_gatt_exchange_mtu(conn, &multirole_mtu_exchange_params_peripheral);
	}
}

static void multirole_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info conn_info;
	int err;

	if (conn != central_conn && conn != peripheral_conn) {
		return;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("[Multirole] Failed to get connection info (%d)\n", err);
		return;
	}

	printk("[Multirole] Disconnected: %s role %u (reason 0x%02x)\n",
	       bt_conn_dst_str(conn), conn_info.role, reason);

	if (conn == central_conn) {
		bt_conn_unref(central_conn);
		central_conn = NULL;
		central_subscribed = false;
		/* Restart scanning to reconnect to Device 3 */
		multirole_start_scan();
	} else if (conn == peripheral_conn) {
		bt_conn_unref(peripheral_conn);
		peripheral_conn = NULL;
		peripheral_notify_enabled = false;
		/* Restart advertising for Device 1 */
		err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, multirole_ad,
				      ARRAY_SIZE(multirole_ad), NULL, 0);
		if (err) {
			printk("[Multirole] [Peripheral] Advertising failed to restart (err %d)\n",
			       err);
		}
	}
}

static void multirole_le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("[Multirole] Conn params updated: int 0x%04x lat %u to %u\n", interval,
	       latency, timeout);
}

BT_CONN_CB_DEFINE(multirole_conn_cb) = {
	.connected = multirole_connected,
	.disconnected = multirole_disconnected,
	.le_param_updated = multirole_le_param_updated,
};

/* ===================== Main entry point ===================== */

uint32_t central_peripheral_notify(uint32_t count)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("[Multirole] Bluetooth init failed (err %d)\n", err);
		return 0U;
	}
	printk("[Multirole] Bluetooth initialized\n");

	bt_gatt_cb_register(&multirole_gatt_callbacks);

	multirole_active = true;
	central_conn = NULL;
	peripheral_conn = NULL;
	peripheral_notify_enabled = false;
	central_subscribed = false;
	multirole_last_tx_notify_rate = 0U;
	multirole_last_rx_notify_rate = 0U;
	multirole_notify_countdown = &count;

	/* Start advertising (peripheral role - for Device 1 to connect) */
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, multirole_ad,
			      ARRAY_SIZE(multirole_ad), NULL, 0);
	if (err) {
		printk("[Multirole] Advertising failed to start (err %d)\n", err);
		return 0U;
	}
	printk("[Multirole] [Peripheral] Advertising successfully started\n");

	/* Start scanning (central role - to connect to Device 3) */
	multirole_start_scan();

	if (count != 0U) {
		printk("[Multirole] Notify relay countdown %u.\n", count);
	} else {
		printk("[Multirole] Notify relay forever.\n");
	}

	/* Main loop: send notifications to Device 1 as fast as possible */
	while (true) {
		struct bt_conn *conn = NULL;

		if (peripheral_conn && peripheral_notify_enabled) {
			conn = bt_conn_ref(peripheral_conn);
		}

		if (conn) {
			int ret = multirole_send_notification(conn);

			bt_conn_unref(conn);

			if (ret > 0) {
				update_tx_metrics((uint16_t)ret);

				if (count && !*multirole_notify_countdown) {
					break;
				}
			} else if (ret == -ENOMEM) {
				k_yield();
			}

			k_yield();
		} else {
			k_sleep(K_SECONDS(1));
		}
	}

	return multirole_last_tx_notify_rate;
}
