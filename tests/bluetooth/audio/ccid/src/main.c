/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/bluetooth/audio/ccid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/ztest_test.h>
#include <zephyr/ztest_assert.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(audio_ccid_test_suite, NULL, NULL, NULL, NULL, NULL);

#define MAX_CCID_CNT 256

static ZTEST(audio_ccid_test_suite, test_bt_ccid_alloc_value)
{
	const int ret = bt_ccid_alloc_value();

	zassert_true(ret >= 0 && ret <= UINT8_MAX, "Unexpected return value %d", ret);
}

static ZTEST(audio_ccid_test_suite, test_bt_ccid_alloc_value_more_than_max)
{
	/* Verify that we can allocate more than max CCID if they are not registered */
	for (uint16_t i = 0U; i < MAX_CCID_CNT * 2; i++) {
		const int ret = bt_ccid_alloc_value();

		zassert_true(ret >= 0 && ret <= UINT8_MAX, "Unexpected return value %d", ret);
	}
}

static ssize_t read_ccid(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	const unsigned int ccid = POINTER_TO_UINT(attr->user_data);
	const uint8_t ccid_u8 = (uint8_t)ccid;

	zassert_true(ccid <= BT_CCID_MAX);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ccid_u8, sizeof(ccid_u8));
}

#define CCID_DEFINE(_n, ...)                                                                       \
	BT_GATT_CHARACTERISTIC(BT_UUID_CCID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_ccid,      \
			       NULL, UINT_TO_POINTER(_n))

/* BT_GATT_PRIMARY_SERVICE only works in the global scope */
static struct bt_gatt_attr test_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_TBS),
	LISTIFY(MAX_CCID_CNT, CCID_DEFINE, (,)),
};

static ZTEST(audio_ccid_test_suite, test_bt_ccid_alloc_value_all_allocated)
{
	struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);
	int ret;

	zassert_ok(bt_gatt_service_register(&test_svc));

	/* Verify that CCID allocation fails if we have 255 characterstics with it */
	ret = bt_ccid_alloc_value();

	zassert_ok(bt_gatt_service_unregister(&test_svc));

	zassert_equal(ret, -ENOMEM, "Unexpected return value %d", ret);
}

static ZTEST(audio_ccid_test_suite, test_bt_ccid_find_attr)
{
	struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);

	/* Service not registered, shall fail */
	zassert_is_null(bt_ccid_find_attr(0));

	zassert_ok(bt_gatt_service_register(&test_svc));

	/* Service registered, shall not fail */
	zassert_not_null(bt_ccid_find_attr(0));

	zassert_ok(bt_gatt_service_unregister(&test_svc));
}
