/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/net/buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/ead.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "common/bt_str.h"

#include "common.h"

LOG_MODULE_REGISTER(ead_central_sample, CONFIG_BT_EAD_LOG_LEVEL);

static uint8_t *received_data;
static size_t received_data_size;
static struct key_material keymat;

static bt_addr_le_t peer_addr;
static struct bt_conn *default_conn;

static struct bt_conn_cb central_cb;
static struct bt_conn_auth_cb central_auth_cb;

static struct k_poll_signal conn_signal;
static struct k_poll_signal sec_update_signal;
static struct k_poll_signal passkey_enter_signal;
static struct k_poll_signal device_found_cb_completed;

/* GATT Discover data */
static uint8_t gatt_disc_err;
static uint16_t gatt_disc_end_handle;
static uint16_t gatt_disc_start_handle;
static struct k_poll_signal gatt_disc_signal;

/* GATT Read data */
static uint8_t gatt_read_err;
static uint8_t *gatt_read_res;
static uint16_t gatt_read_len;
static uint16_t gatt_read_handle;
static struct k_poll_signal gatt_read_signal;

static bool data_parse_cb(struct bt_data *data, void *user_data)
{
	size_t *parsed_data_size = (size_t *)user_data;

	if (data->type == BT_DATA_ENCRYPTED_AD_DATA) {
		int err;
		struct net_buf_simple decrypted_buf;
		size_t decrypted_data_size = BT_EAD_DECRYPTED_PAYLOAD_SIZE(data->data_len);
		uint8_t decrypted_data[decrypted_data_size];

		err = bt_ead_decrypt(keymat.session_key, keymat.iv, data->data, data->data_len,
				     decrypted_data);
		if (err < 0) {
			LOG_ERR("Error during decryption (err %d)", err);
		}

		net_buf_simple_init_with_data(&decrypted_buf, &decrypted_data[0],
					      decrypted_data_size);

		bt_data_parse(&decrypted_buf, &data_parse_cb, user_data);
	} else {
		LOG_INF("len : %u", data->data_len);
		LOG_INF("type: 0x%02x", data->type);
		LOG_HEXDUMP_INF(data->data, data->data_len, "data:");

		/* Copy the data out if we are running in a test context */
		if (received_data != NULL) {
			if (bt_data_get_len(data, 1) <=
			    (received_data_size - (*parsed_data_size))) {
				*parsed_data_size +=
					bt_data_serialize(data, &received_data[*parsed_data_size]);
			}
		} else {
			*parsed_data_size += bt_data_get_len(data, 1);
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;
	size_t parsed_data_size;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	/* We are only interested in the previously connected device. */
	if (!bt_addr_le_eq(addr, &peer_addr)) {
		return;
	}

	LOG_DBG("Peer found.");

	parsed_data_size = 0;
	LOG_INF("Received data size: %zu", ad->len);
	bt_data_parse(ad, data_parse_cb, &parsed_data_size);

	LOG_DBG("All data parsed. (total size: %zu)", parsed_data_size);

	err = bt_le_scan_stop();
	if (err) {
		LOG_DBG("Failed to stop scanner (err %d)", err);
		return;
	}

	k_poll_signal_raise(&device_found_cb_completed, 0);
}

static void connect_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
				 struct net_buf_simple *ad)
{
	int err;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (default_conn) {
		return;
	}

	/* Connect only to devices in close range */
	if (rssi < -70) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	LOG_DBG("Device found: %s (RSSI %d)", addr_str, rssi);

	err = bt_le_scan_stop();
	if (err) {
		LOG_DBG("Failed to stop scanner (err %d)", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		LOG_DBG("Failed to connect to %s (err %d)", addr_str, err);
		return;
	}

	k_poll_signal_raise(&device_found_cb_completed, 0);
}

static int start_scan(bool connect)
{
	int err;

	k_poll_signal_reset(&conn_signal);
	k_poll_signal_reset(&device_found_cb_completed);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, connect ? connect_device_found : device_found);
	if (err) {
		LOG_DBG("Scanning failed to start (err %d)", err);
		return -1;
	}

	LOG_DBG("Scanning started.");

	if (connect) {
		LOG_DBG("Waiting for connection");
		await_signal(&conn_signal);
	}

	await_signal(&device_found_cb_completed);

	return 0;
}

static uint8_t gatt_read_cb(struct bt_conn *conn, uint8_t att_err,
			    struct bt_gatt_read_params *params, const void *data, uint16_t read_len)
{
	gatt_read_err = att_err;
	gatt_read_len = read_len;
	gatt_read_handle = params->by_uuid.start_handle;

	if (!att_err) {
		memcpy(gatt_read_res, data, read_len);
		k_poll_signal_raise(&gatt_read_signal, 0);
	} else {
		LOG_ERR("ATT error (err %d)", att_err);
	}

	return BT_GATT_ITER_STOP;
}

static int gatt_read(struct bt_conn *conn, const struct bt_uuid *uuid, size_t read_size,
		     uint16_t start_handle, uint16_t end_handle, uint8_t *buf)
{
	int err;
	size_t offset;
	uint16_t handle;
	struct bt_gatt_read_params params;

	gatt_read_res = &buf[0];

	params.handle_count = 0;
	params.by_uuid.start_handle = start_handle;
	params.by_uuid.end_handle = end_handle;
	params.by_uuid.uuid = uuid;
	params.func = gatt_read_cb;

	k_poll_signal_reset(&gatt_read_signal);

	err = bt_gatt_read(conn, &params);
	if (err) {
		LOG_DBG("GATT read failed (err %d)", err);
		return -1;
	}

	await_signal(&gatt_read_signal);

	offset = gatt_read_len;
	handle = gatt_read_handle;

	while (offset < read_size) {
		gatt_read_res = &buf[offset];

		params.handle_count = 1;
		params.single.handle = handle;
		params.single.offset = offset;

		k_poll_signal_reset(&gatt_read_signal);

		err = bt_gatt_read(conn, &params);
		if (err) {
			LOG_DBG("GATT read failed (err %d)", err);
			return -1;
		}

		await_signal(&gatt_read_signal);

		offset += gatt_read_len;
	}

	return 0;
}

static uint8_t gatt_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	gatt_disc_err = attr ? 0 : BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

	if (attr) {
		gatt_disc_start_handle = attr->handle;
		gatt_disc_end_handle = ((struct bt_gatt_service_val *)attr->user_data)->end_handle;
	}

	k_poll_signal_raise(&gatt_disc_signal, 0);

	return BT_GATT_ITER_STOP;
}

static int gatt_discover_primary_service(struct bt_conn *conn, const struct bt_uuid *service_type,
					 uint16_t *start_handle, uint16_t *end_handle)
{
	int err;
	struct bt_gatt_discover_params params;

	params.type = BT_GATT_DISCOVER_PRIMARY;
	params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params.uuid = service_type;
	params.func = gatt_discover_cb;

	k_poll_signal_reset(&gatt_disc_signal);

	err = bt_gatt_discover(conn, &params);
	if (err) {
		LOG_DBG("Primary service discover failed (err %d)", err);
		return -1;
	}

	await_signal(&gatt_disc_signal);

	*start_handle = gatt_disc_start_handle;
	*end_handle = gatt_disc_end_handle;

	return gatt_disc_err;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_DBG("Failed to connect to %s (err %u)", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		(void)start_scan(true);

		return;
	}

	LOG_DBG("Connected to: %s", addr);

	k_poll_signal_raise(&conn_signal, 0);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_DBG("Security changed: %s level %u", addr, level);
		k_poll_signal_raise(&sec_update_signal, 0);
	} else {
		LOG_DBG("Security failed: %s level %u err %d", addr, level, err);
	}
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	LOG_DBG("Identity resolved %s -> %s", addr_rpa, addr_identity);

	bt_addr_le_copy(&peer_addr, identity);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char passkey_str[7];
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, ARRAY_SIZE(passkey_str), "%06u", passkey);

	printk("Passkey for %s: %s\n", addr, passkey_str);

	k_poll_signal_raise(&passkey_enter_signal, 0);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char passkey_str[7];
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, ARRAY_SIZE(passkey_str), "%06u", passkey);

	LOG_DBG("Passkey for %s: %s", addr, passkey_str);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Pairing cancelled: %s", addr);
}

static int init_bt(void)
{
	int err;

	default_conn = NULL;

	k_poll_signal_init(&conn_signal);
	k_poll_signal_init(&passkey_enter_signal);
	k_poll_signal_init(&sec_update_signal);
	k_poll_signal_init(&gatt_disc_signal);
	k_poll_signal_init(&gatt_read_signal);
	k_poll_signal_init(&device_found_cb_completed);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return -1;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err) {
		LOG_ERR("Unpairing failed (err %d)", err);
	}

	central_cb.connected = connected;
	central_cb.disconnected = disconnected;
	central_cb.security_changed = security_changed;
	central_cb.identity_resolved = identity_resolved;

	bt_conn_cb_register(&central_cb);

	central_auth_cb.pairing_confirm = NULL;
	central_auth_cb.passkey_confirm = auth_passkey_confirm;
	central_auth_cb.passkey_display = auth_passkey_display;
	central_auth_cb.passkey_entry = NULL;
	central_auth_cb.oob_data_request = NULL;
	central_auth_cb.cancel = auth_cancel;

	err = bt_conn_auth_cb_register(&central_auth_cb);
	if (err) {
		return -1;
	}

	return 0;
}

int run_central_sample(int get_passkey_confirmation(struct bt_conn *conn),
		       uint8_t *test_received_data, size_t test_received_data_size,
		       struct key_material *test_received_keymat)
{
	int err;
	bool connect;
	uint16_t end_handle;
	uint16_t start_handle;

	if (test_received_data != NULL) {
		received_data = test_received_data;
		received_data_size = test_received_data_size;
	}

	/* Initialize Bluetooth and callbacks */
	err = init_bt();
	if (err) {
		return -1;
	}

	/* Start scan and connect to our peripheral */
	connect = true;
	err = start_scan(connect);
	if (err) {
		return -2;
	}

	/* Update connection security level */
	err = bt_conn_set_security(default_conn, BT_SECURITY_L4);
	if (err) {
		LOG_ERR("Failed to set security (err %d)", err);
		return -3;
	}

	await_signal(&passkey_enter_signal);

	err = get_passkey_confirmation(default_conn);
	if (err) {
		LOG_ERR("Security update failed");
		return -4;
	}

	/* Locate the primary service */
	err = gatt_discover_primary_service(default_conn, BT_UUID_CUSTOM_SERVICE, &start_handle,
					    &end_handle);
	if (err) {
		LOG_ERR("Service not found (err %d)", err);
		return -5;
	}

	/* Read the Key Material characteristic */
	err = gatt_read(default_conn, BT_UUID_GATT_EDKM, sizeof(keymat), start_handle, end_handle,
			(uint8_t *)&keymat);
	if (err) {
		LOG_ERR("GATT read failed (err %d)", err);
		return -6;
	}

	LOG_HEXDUMP_DBG(keymat.session_key, BT_EAD_KEY_SIZE, "Session Key");
	LOG_HEXDUMP_DBG(keymat.iv, BT_EAD_IV_SIZE, "IV");

	if (test_received_keymat != NULL) {
		memcpy(test_received_keymat, &keymat, sizeof(keymat));
	}

	/* Start a new scan to get and decrypt the Advertising Data */
	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		LOG_ERR("Failed to disconnect.");
		return -7;
	}

	connect = false;
	err = start_scan(connect);
	if (err) {
		return -2;
	}

	return 0;
}
