/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * GATT test service for the ATT bearer over BR/EDR. The pytest harness drives
 * the Bluetooth shell; this file only adds static services with known UUIDs:
 * a service published in SDP with a readable, writable and notifiable value
 * with a CCC and a value that needs an authenticated link to be read, and a
 * service without SDP record.
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define TEST_SVC_UUID                                                                              \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x9b0a0000, 0x7c1e, 0x4c64, 0x9a3b, 0x5f2d1e8c0a01))
#define TEST_VAL_UUID                                                                              \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x9b0a0001, 0x7c1e, 0x4c64, 0x9a3b, 0x5f2d1e8c0a01))
#define TEST_AUTH_UUID                                                                             \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x9b0a0002, 0x7c1e, 0x4c64, 0x9a3b, 0x5f2d1e8c0a01))
#define TEST_HIDDEN_SVC_UUID                                                                       \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x9b0a0010, 0x7c1e, 0x4c64, 0x9a3b, 0x5f2d1e8c0a01))
#define TEST_HIDDEN_VAL_UUID                                                                       \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x9b0a0011, 0x7c1e, 0x4c64, 0x9a3b, 0x5f2d1e8c0a01))

static uint8_t test_value[16] = "att_br";
static size_t test_value_len = sizeof("att_br") - 1U;

static ssize_t read_value(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, test_value, test_value_len);
}

static ssize_t write_value(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			   uint16_t len, uint16_t offset, uint8_t flags)
{
	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > sizeof(test_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	(void)memcpy(test_value, buf, len);
	test_value_len = len;

	return len;
}

static ssize_t read_auth(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	static const uint8_t auth_value[] = {0x01};

	return bt_gatt_attr_read(conn, attr, buf, len, offset, auth_value, sizeof(auth_value));
}

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SVC_UUID),
	BT_GATT_CHARACTERISTIC(TEST_VAL_UUID,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_value, write_value,
			       NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(TEST_AUTH_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_AUTHEN,
			       read_auth, NULL, NULL),
);
BT_ATT_BR_SDP_SERVICE(test_svc, TEST_SVC_UUID);

/* Not defined with BT_ATT_BR_SDP_SERVICE(): no SDP record, ATT discovery only */
BT_GATT_SERVICE_DEFINE(test_hidden_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_HIDDEN_SVC_UUID),
	BT_GATT_CHARACTERISTIC(TEST_HIDDEN_VAL_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_value, NULL, NULL),
);
