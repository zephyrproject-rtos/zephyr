/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <string.h>

#include <zephyr/display/mb_display.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>


#include "pong.h"

#define SCAN_TIMEOUT     K_SECONDS(2)

#define APPEARANCE       0

#define PONG_SVC_UUID \
	BT_UUID_128_ENCODE(0xf94fea38, 0x4e24, 0x7ea1, 0x0d4d, 0x6fee0f556c90)
#define PONG_CHR_UUID \
	BT_UUID_128_ENCODE(0xabbf8f1c, 0xc56a, 0x82b5, 0xc640, 0x2ccdd7af94dd)

static struct bt_uuid_128 pong_svc_uuid = BT_UUID_INIT_128(PONG_SVC_UUID);
static struct bt_uuid_128 pong_chr_uuid = BT_UUID_INIT_128(PONG_CHR_UUID);
static struct bt_uuid *gatt_ccc_uuid = BT_UUID_GATT_CCC;

static struct bt_gatt_discover_params discov_param;
static struct bt_gatt_subscribe_params subscribe_param;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, PONG_SVC_UUID),
};

static struct bt_conn *default_conn;

static const struct bt_gatt_attr *local_attr;
static uint16_t remote_handle;
static bool remote_ready;
static bool initiator;

static struct k_work_delayable ble_work;

static bool connect_canceled;

static enum {
	BLE_DISCONNECTED,
	BLE_SCAN_START,
	BLE_SCAN,
	BLE_CONNECT_CREATE,
	BLE_CONNECT_CANCEL,
	BLE_ADV_START,
	BLE_ADVERTISING,
	BLE_CONNECTED,
} ble_state;

enum {
	BLE_BALL_INFO = 0x00,
	BLE_LOST = 0x01,
};

struct ble_ball_info {
	int8_t x_pos;
	int8_t y_pos;
	int8_t x_vel;
	int8_t y_vel;
} __packed;

struct ble_data {
	uint8_t op;
	union {
		struct ble_ball_info ball;
	};
} __packed;

#define BALL_INFO_LEN (1 + sizeof(struct ble_ball_info))

void ble_send_ball(int8_t x_pos, int8_t y_pos, int8_t x_vel, int8_t y_vel)
{
	struct ble_data data = {
		.op           = BLE_BALL_INFO,
		.ball.x_pos   = x_pos,
		.ball.y_pos   = y_pos,
		.ball.x_vel   = x_vel,
		.ball.y_vel   = y_vel,
	};
	int err;

	if (!default_conn || !remote_ready) {
		printk("ble_send_ball(): not ready\n");
		return;
	}

	printk("ble_send_ball(%d, %d, %d, %d)\n", x_pos, y_pos, x_vel, y_vel);

	err = bt_gatt_notify(default_conn, local_attr, &data, BALL_INFO_LEN);
	if (err) {
		printk("GATT notify failed (err %d)\n", err);
	}
}

void ble_send_lost(void)
{
	uint8_t lost = BLE_LOST;
	int err;

	if (!default_conn || !remote_ready) {
		printk("ble_send_lost(): not ready\n");
		return;
	}

	err = bt_gatt_notify(default_conn, local_attr, &lost, sizeof(lost));
	if (err) {
		printk("GATT notify failed (err %d)\n", err);
	}
}

static uint8_t notify_func(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *param,
			const void *buf, uint16_t len)
{
	const struct ble_data *data = buf;

	printk("notify_func() data %p len %u\n", data, len);

	if (!data || !len) {
		printk("Unsubscribed, disconnecting...\n");
		remote_handle = 0U;
		if (default_conn) {
			bt_conn_disconnect(default_conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
		return BT_GATT_ITER_STOP;
	}

	switch (data->op) {
	case BLE_BALL_INFO:
		if (len < BALL_INFO_LEN) {
			printk("Too small ball info\n");
			break;
		}

		pong_ball_received(data->ball.x_pos, data->ball.y_pos,
				   data->ball.x_vel, data->ball.y_vel);
		break;
	case BLE_LOST:
		pong_remote_lost();
		break;
	default:
		printk("Unknown op 0x%02x\n", data->op);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  struct bt_gatt_discover_params *param)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(&discov_param, 0, sizeof(discov_param));
		return BT_GATT_ITER_STOP;
	}

	printk("Attribute handle %u\n", attr->handle);

	if (param->uuid == &pong_svc_uuid.uuid) {
		printk("Pong service discovered\n");
		discov_param.uuid = &pong_chr_uuid.uuid;
		discov_param.start_handle = attr->handle + 1;
		discov_param.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discov_param);
		if (err) {
			printk("Char Discovery failed (err %d)\n", err);
		}
	} else if (param->uuid == &pong_chr_uuid.uuid) {
		printk("Pong characteristic discovered\n");
		discov_param.uuid = gatt_ccc_uuid;
		discov_param.start_handle = attr->handle + 2;
		discov_param.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_param.value_handle = attr->handle + 1;

		err = bt_gatt_discover(conn, &discov_param);
		if (err) {
			printk("CCC Discovery failed (err %d)\n", err);
		}
	} else {
		printk("Pong CCC discovered\n");

		subscribe_param.notify = notify_func;
		subscribe_param.value = BT_GATT_CCC_NOTIFY;
		subscribe_param.ccc_handle = attr->handle;

		printk("CCC handle 0x%04x Value handle 0x%04x\n",
		       subscribe_param.ccc_handle,
		       subscribe_param.value_handle);

		err = bt_gatt_subscribe(conn, &subscribe_param);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("Subscribed\n");
		}

		remote_handle = attr->handle;
	}

	if (remote_handle && remote_ready) {
		pong_conn_ready(initiator);
	}

	return BT_GATT_ITER_STOP;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
		return;
	}

	if (ble_state == BLE_ADVERTISING) {
		bt_le_adv_stop();
	}

	if (!default_conn) {
		default_conn = bt_conn_ref(conn);
	}

	bt_conn_get_info(conn, &info);
	initiator = (info.role == BT_CONN_ROLE_CENTRAL);
	remote_ready = false;
	remote_handle = 0U;

	printk("Connected\n");
	ble_state = BLE_CONNECTED;

	k_work_reschedule(&ble_work, K_NO_WAIT);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	remote_handle = 0U;

	if (ble_state == BLE_CONNECTED) {
		ble_state = BLE_DISCONNECTED;
		pong_remote_disconnected();
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

void ble_connect(void)
{
	if (ble_state != BLE_DISCONNECTED) {
		printk("Not ready to connect\n");
		return;
	}

	ble_state = BLE_SCAN_START;
	k_work_reschedule(&ble_work, K_NO_WAIT);
}

void ble_cancel_connect(void)
{
	printk("ble_cancel_connect()\n");

	switch (ble_state) {
	case BLE_SCAN_START:
		ble_state = BLE_DISCONNECTED;
		__fallthrough;
	case BLE_DISCONNECTED:
		/* If this fails, the handler will run without doing anything,
		 * as the switch case for BLE_DISCONNECTED is empty.
		 */
		k_work_cancel_delayable(&ble_work);
		break;
	case BLE_SCAN:
		connect_canceled = true;
		k_work_reschedule(&ble_work, K_NO_WAIT);
		break;
	case BLE_ADV_START:
		ble_state = BLE_DISCONNECTED;
		break;
	case BLE_ADVERTISING:
		connect_canceled = true;
		k_work_reschedule(&ble_work, K_NO_WAIT);
		break;
	case BLE_CONNECT_CREATE:
		ble_state = BLE_CONNECT_CANCEL;
		__fallthrough;
	case BLE_CONNECTED:
		connect_canceled = true;
		k_work_reschedule(&ble_work, K_NO_WAIT);
		break;
	case BLE_CONNECT_CANCEL:
		break;
	}
}

static bool pong_uuid_match(const uint8_t *data, uint8_t len)
{
	while (len >= 16U) {
		if (!memcmp(data, pong_svc_uuid.val, 16)) {
			return true;
		}

		len -= 16U;
		data += 16;
	}

	return false;
}

static void create_conn(const bt_addr_le_t *addr)
{
	int err;

	if (default_conn) {
		return;
	}

	printk("Found matching device, initiating connection...\n");

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err) {
		printk("Failed to initiate connection");
		return;
	}

	ble_state = BLE_CONNECT_CREATE;
	k_work_reschedule(&ble_work, SCAN_TIMEOUT);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	if (type != BT_GAP_ADV_TYPE_ADV_IND) {
		return;
	}

	while (ad->len > 1) {
		uint8_t len = net_buf_simple_pull_u8(ad);
		uint8_t type;

		/* Check for early termination */
		if (len == 0U) {
			return;
		}

		if (len > ad->len) {
			printk("AD malformed\n");
			return;
		}

		type = net_buf_simple_pull_u8(ad);
		if (type == BT_DATA_UUID128_ALL &&
		    pong_uuid_match(ad->data, len - 1)) {
			bt_le_scan_stop();
			create_conn(addr);
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

static uint32_t adv_timeout(void)
{
	uint32_t timeout;

	if (bt_rand(&timeout, sizeof(timeout)) < 0) {
		return 10 * MSEC_PER_SEC;
	}

	timeout %= (10 * MSEC_PER_SEC);

	return timeout + (1 * MSEC_PER_SEC);
}

static void cancel_connect(void)
{
	connect_canceled = false;

	switch (ble_state) {
	case BLE_SCAN:
		bt_le_scan_stop();
		break;
	case BLE_ADVERTISING:
		bt_le_adv_stop();
		break;
	case BLE_CONNECT_CREATE:
	case BLE_CONNECTED:
		bt_conn_disconnect(default_conn,
				   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		break;
	default:
		break;
	}

	/* For CONNECTED the state will be updated in the disconnected cb */
	if (ble_state != BLE_CONNECTED) {
		ble_state = BLE_DISCONNECTED;
	}
}

static void ble_timeout(struct k_work *work)
{
	int err;

	if (connect_canceled) {
		cancel_connect();
		return;
	}

	switch (ble_state) {
	case BLE_DISCONNECTED:
		break;
	case BLE_SCAN_START:
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
		}

		printk("Started scanning for devices\n");
		ble_state = BLE_SCAN;
		k_work_reschedule(&ble_work, SCAN_TIMEOUT);
		break;
	case BLE_CONNECT_CREATE:
		printk("Connection attempt timed out\n");
		bt_conn_disconnect(default_conn,
				   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		ble_state = BLE_ADV_START;
		k_work_reschedule(&ble_work, K_NO_WAIT);
		break;
	case BLE_SCAN:
		printk("No devices found during scan\n");
		bt_le_scan_stop();
		ble_state = BLE_ADV_START;
		k_work_reschedule(&ble_work, K_NO_WAIT);
		break;
	case BLE_ADV_START:
		err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad),
				      NULL, 0);
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return;
		}

		printk("Advertising successfully started\n");
		ble_state = BLE_ADVERTISING;
		k_work_reschedule(&ble_work, K_MSEC(adv_timeout()));
		break;
	case BLE_ADVERTISING:
		printk("Timed out advertising\n");
		bt_le_adv_stop();
		ble_state = BLE_SCAN_START;
		k_work_reschedule(&ble_work, K_NO_WAIT);
		break;
	case BLE_CONNECTED:
		discov_param.uuid = &pong_svc_uuid.uuid;
		discov_param.func = discover_func;
		discov_param.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discov_param.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discov_param.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(default_conn, &discov_param);
		if (err) {
			printk("Discover failed (err %d)\n", err);
			return;
		}
		break;
	case BLE_CONNECT_CANCEL:
		break;
	}
}

static void pong_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t val)
{
	printk("val %u\n", val);

	remote_ready = (val == BT_GATT_CCC_NOTIFY);

	if (remote_ready && remote_handle) {
		pong_conn_ready(initiator);
	}
}

BT_GATT_SERVICE_DEFINE(pong_svc,
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&pong_svc_uuid.uuid),
	BT_GATT_CHARACTERISTIC(&pong_chr_uuid.uuid, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(pong_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void ble_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Enabling Bluetooth failed (err %d)\n", err);
		return;
	}

	k_work_init_delayable(&ble_work, ble_timeout);

	local_attr = &pong_svc.attrs[1];
}
