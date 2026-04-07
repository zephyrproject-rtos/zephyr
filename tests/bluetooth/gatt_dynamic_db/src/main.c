/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

static const struct bt_uuid_128 test_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34,
	0x12);
static const struct bt_uuid_128 test_chrc_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34,
	0x12);

static const struct bt_uuid_128 test1_uuid = BT_UUID_INIT_128(
	0xf4, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34,
	0x12);
static const struct bt_uuid_128 test1_chrc_uuid = BT_UUID_INIT_128(
	0xf5, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34,
	0x12);

static uint8_t test_value[] = {'T', 'e', 's', 't', '\0'};

static ssize_t read_test(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

static struct bt_gatt_attr test_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&test_uuid),
	BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_test, NULL, test_value),
};

static struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);

static struct bt_gatt_attr test1_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&test1_uuid),
	BT_GATT_CHARACTERISTIC(&test1_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_test, NULL, test_value),
};

static struct bt_gatt_service test1_svc = BT_GATT_SERVICE(test1_attrs);

ZTEST_SUITE(gatt_dynamic_db, NULL, NULL, NULL, NULL, NULL);

/*
 * Dynamic DB with Service Changed disabled: register/unregister are only
 * allowed before bt_enable(); after the stack is initialized these APIs return
 * -ENOTSUP.
 */
ZTEST(gatt_dynamic_db, test_register_unregister_enotsup_after_bt_enable)
{
	int err;

	if (bt_is_ready()) {
		err = bt_disable();
		zassert_equal(err, 0, "bt_disable failed (%d)", err);
	}

	bt_gatt_service_unregister(&test_svc);
	bt_gatt_service_unregister(&test1_svc);

	err = bt_gatt_service_register(&test_svc);
	zassert_equal(err, 0, "register before bt_enable failed (%d)", err);

	err = bt_enable(NULL);
	zassert_equal(err, 0, "bt_enable failed (%d)", err);

	err = bt_gatt_service_register(&test1_svc);
	zassert_equal(err, -ENOTSUP, "register after init: expected ENOTSUP, got %d", err);

	err = bt_gatt_service_unregister(&test_svc);
	zassert_equal(err, -ENOTSUP, "unregister after init: expected ENOTSUP, got %d", err);

	err = bt_disable();
	zassert_equal(err, 0, "bt_disable failed (%d)", err);
}
