/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>

#ifdef TRACK_VND
#ifndef TRACK_READ_CNT
#define TRACK_READ_CNT	300
#endif /* TRACK_READ_CNT */
#endif /* TRACK_VND */

#define STATE_NULL	0
#define STATE_DISCOV	1
#define STATE_READING	2
#define STATE_DONE	3
#define STATE_RAISELINK	4

#define TOPIC_CHRC_DECL	1
#define TOPIC_CHRC_VAL	2

#ifdef TRACK_VND
struct gatt_chrc {
	u8_t properties;
	u16_t value_handle;
	union {
		u16_t uuid16;
		u8_t  uuid[16];
	};
} __packed;

static struct bt_gatt_read_params read_params;
static u8_t read_topic;
static u16_t value_handle;
static u16_t value_len;
static u8_t value_data[100];
static u16_t read_cnt;
static volatile u8_t state_flag;

static struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
		0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static struct bt_uuid_128 vnd_enc_uuid = BT_UUID_INIT_128(
		0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
#endif

static struct bt_conn *default_conn;

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static u8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, u16_t length)
{
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	printk("[NOTIFICATION] data %p length %u\n", data, length);

	return BT_GATT_ITER_CONTINUE;
}

#ifdef TRACK_VND
static u8_t read_func_cb(struct bt_conn *conn, u8_t err,
		struct bt_gatt_read_params *params, const void *data,
		u16_t length)
{
	if (err == BT_ATT_ERR_AUTHENTICATION ||
	    err == BT_ATT_ERR_AUTHORIZATION ||
	    err == BT_ATT_ERR_INSUFFICIENT_ENCRYPTION) {
		state_flag = STATE_RAISELINK;
		return BT_GATT_ITER_STOP;
	} else if (err != 0) {
		printk("ATT error %d\n", err);
		state_flag = STATE_NULL;
		return BT_GATT_ITER_STOP;
	}

	if (!data) {
		state_flag = STATE_DONE;
		return BT_GATT_ITER_STOP;
	}

	if (read_topic == TOPIC_CHRC_DECL) {
		const struct gatt_chrc *value = data;

		value_handle = sys_le16_to_cpu(value->value_handle);
	} else if (read_topic == TOPIC_CHRC_VAL) {
		if (length < sizeof(value_data)) {
			value_len = length;
		} else {
			value_len = sizeof(value_data);
		}

		memcpy(value_data, data, value_len);
	}

	return BT_GATT_ITER_CONTINUE;
}
#endif /* TRACK_VND */

static u8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	const u8_t *type[3] = {"UUID16", "UUID32", "UUID128"};

	if (discover_params.uuid->type < 3) {
		printk("[ATTRIBUTE] %s handle %u\n",
		       type[discover_params.uuid->type], attr->handle);
	}

	if (0) {
#ifdef TRACK_VND
	} else if (!bt_uuid_cmp(discover_params.uuid, &vnd_enc_uuid.uuid)) {
		printk("Found Vendor Specific attribute\n");
		read_cnt = 0;
		value_handle = attr->handle;
		state_flag = STATE_DISCOV;
#endif /* TRACK_VND */
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HRS)) {

#ifdef TRACK_VND
		discover_params.uuid = &vnd_enc_uuid.uuid;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
#else /* !TRACK_VND */
		memcpy(&uuid, BT_UUID_HRS_MEASUREMENT, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
#endif /* TRACK_VND */

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid,
				BT_UUID_HRS_MEASUREMENT)) {
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else {
		subscribe_params.notify = notify_func;
#ifndef TRACK_VND
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}
#endif /* !TRACK_VND */

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

	if (conn == default_conn) {
		memcpy(&uuid, BT_UUID_HRS, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = 0x0001;
		discover_params.end_handle = 0xffff;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(default_conn, &discover_params);
		if (err) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int err;
	int i;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {
#ifdef TRACK_VND
	case BT_DATA_UUID128_SOME:
	case BT_DATA_UUID128_ALL:
		if (data->data_len % sizeof(vnd_uuid.val) != 0U) {
			printk("AD malformed\n");
			return true;
		}

		if (memcmp(data->data, &vnd_uuid.val, sizeof(vnd_uuid.val))) {
			break;
		}

		printk("[AD]: Found Vendor Specific UUID\n");
		err = bt_le_scan_stop();
		if (err) {
			printk("Stop LE scan failed (err %d)\n", err);
			return true;
		}

		default_conn = bt_conn_create_le(addr,
						 BT_LE_CONN_PARAM_DEFAULT);
		return false;
#endif /* TRACK_VND */
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(u16_t) != 0U) {
			printk("AD malformed\n");
			return true;
		}

		for (i = 0; i < data->data_len; i += sizeof(u16_t)) {
			struct bt_uuid *uuid;
			u16_t u16;

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

			default_conn = bt_conn_create_le(addr,
							 BT_LE_CONN_PARAM_DEFAULT);
			return false;
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	       dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_LE_ADV_IND || type == BT_LE_ADV_DIRECT_IND) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static int scan_start(void)
{
	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime. */
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_ACTIVE,
		.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	return bt_le_scan_start(&scan_param, device_found);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
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
#ifdef TRACK_VND
	state_flag = STATE_NULL;
#endif /* TRACK_VND */

	err = scan_start();
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

#ifdef TRACK_VND
void passkey_entry(struct bt_conn *conn)
{
	printk("input key: 111111\n");
	bt_conn_auth_passkey_entry(conn, 111111);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Pairing cancelled: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	printk("Pairing Complete\n");
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	printk("Pairing Failed (%d). Disconnecting.\n", reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = NULL,
	.passkey_entry = passkey_entry,
	.cancel = auth_cancel,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};
#endif /* TRACK_VND */

#ifdef TRACK_VND
void track_vnd_loop(void)
{
	int err;

	while (1) {
		switch (state_flag) {
		case STATE_NULL:
		case STATE_READING:
			k_sleep(20);
			break;
		case STATE_DISCOV:
			/* Initially read the characteristic declaration */
			read_params.func = read_func_cb;
			read_params.handle_count = 1;
			read_params.single.handle =
						 sys_le16_to_cpu(value_handle);
			read_params.single.offset = sys_le16_to_cpu(0);
			read_topic = TOPIC_CHRC_DECL;

			err = bt_gatt_read(default_conn, &read_params);
			if (err == 0) {
				state_flag = STATE_READING;
			} else {
				printk("read error %d\n", err);
			}

			break;
		case STATE_DONE:
			if (read_topic == TOPIC_CHRC_DECL) {
				/* Read the characteristic declaration before,
				 * now read the value declaration with handle+1
				 */
				read_params.single.handle += 1;
				read_topic = TOPIC_CHRC_VAL;

				err = bt_gatt_read(default_conn, &read_params);
				if (err == 0) {
					state_flag = STATE_READING;
				} else {
					printk("read error %d\n", err);
				}
			} else if (read_topic == TOPIC_CHRC_VAL) {
				int i;

				if (value_len) {
					printk("read: ");
					for (i = 0; i < value_len - 1; i++) {
						printk("%02x", value_data[i]);
					}
					printk("%02x\n", value_data[i]);
				}

				state_flag = STATE_NULL;
				if (read_cnt++ < TRACK_READ_CNT) {
					err = bt_gatt_read(default_conn,
							   &read_params);
					if (err == 0) {
						state_flag = STATE_READING;
					} else {
						printk("read error %d\n", err);
					}
				} else {
					bt_conn_disconnect(default_conn, 0);
				}
			}

			break;
		case STATE_RAISELINK:
			err = bt_conn_set_security(default_conn,
						   BT_SECURITY_L4);
			if (err == 0) {
				state_flag = STATE_DONE;
			} else {
				printk("set security error %d\n", err);
				state_flag = STATE_NULL;
			}
		}
	}
}
#endif /* TRACK_VND */

void main(void)
{
	int err;
	err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

#ifdef TRACK_VND
	bt_conn_auth_cb_register(&auth_cb_display);
#endif /* TRACK_VND */
	bt_conn_cb_register(&conn_callbacks);

	err = scan_start();

	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

#ifdef TRACK_VND
	track_vnd_loop();
#endif /* TRACK_VND */
}
