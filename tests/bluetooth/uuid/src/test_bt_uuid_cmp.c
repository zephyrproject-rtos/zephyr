/* Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/bluetooth/uuid.h>

static struct bt_uuid_16 uuid_16 = BT_UUID_INIT_16(0xffff);

static struct bt_uuid_128 uuid_128 = BT_UUID_INIT_128(
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00);

ZTEST_SUITE(bt_uuid_cmp, NULL, NULL, NULL, NULL, NULL);

ZTEST(bt_uuid_cmp, test_uuid_cmp)
{
	/* Compare UUID 16 bits */
	zassert_false(bt_uuid_cmp(&uuid_16.uuid, BT_UUID_DECLARE_16(0xffff)),
		     "Test UUIDs don't match");

	/* Compare UUID 128 bits */
	zassert_false(bt_uuid_cmp(&uuid_128.uuid, BT_UUID_DECLARE_16(0xffff)),
		     "Test UUIDs don't match");

	/* Compare UUID 16 bits with UUID 128 bits */
	zassert_false(bt_uuid_cmp(&uuid_16.uuid, &uuid_128.uuid),
		     "Test UUIDs don't match");

	/* Compare different UUID 16 bits */
	zassert_true(bt_uuid_cmp(&uuid_16.uuid, BT_UUID_DECLARE_16(0x0000)),
		     "Test UUIDs match");

	/* Compare different UUID 128 bits */
	zassert_true(bt_uuid_cmp(&uuid_128.uuid, BT_UUID_DECLARE_16(0x000)),
		     "Test UUIDs match");
}
